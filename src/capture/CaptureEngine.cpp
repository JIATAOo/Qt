#include "CaptureEngine.h"
#include <obs-data.h>
#include <obs-properties.h>
#include <obs-source.h>
#include <cstring>
#include <thread>

// 必须在跨线程 queued 连接前注册元类型
static const int kVideoFrameMetaTypeId = qRegisterMetaType<Capture::VideoFrame>("Capture::VideoFrame");
static const int kAudioFrameMetaTypeId = qRegisterMetaType<Capture::AudioFrame>("Capture::AudioFrame");

namespace Capture {

static const char *kD3D11  = "libobs-d3d11.dll";
static const char *kOpenGL = "libobs-opengl.dll";

// ──────────────────── 生命周期 ────────────────────

CaptureEngine::CaptureEngine(QObject *parent)
    : QObject(parent)
{
}

CaptureEngine::~CaptureEngine()
{
    shutdown();
}

bool CaptureEngine::init(int sampleRate, int channels, int width, int height, int fps)
{
    if (initialized_) return true;

    sampleRate_ = sampleRate;
    channels_   = channels;

    // 1. 启动 OBS
    if (!obs_startup("en-US", nullptr, nullptr)) {
        emit errorOccurred(tr("OBS startup failed"));
        return false;
    }

    // 2. 加载模块
    obs_module_failure_info mfi;
    obs_load_all_modules2(&mfi);
    obs_post_load_modules();

    // 3. 初始化视频（优先 D3D11，回退 OpenGL）
    obs_video_info vi = {};
    vi.graphics_module = kD3D11;
    vi.base_width      = width;
    vi.base_height     = height;
    vi.output_width    = width;
    vi.output_height   = height;
    vi.fps_num         = fps;
    vi.fps_den         = 1;
    vi.output_format   = VIDEO_FORMAT_I420;
    vi.colorspace      = VIDEO_CS_709;
    vi.range           = VIDEO_RANGE_FULL;
    vi.adapter         = 0;
    vi.gpu_conversion  = true;

    int vr = obs_reset_video(&vi);
    if (vr != OBS_VIDEO_SUCCESS) {
        blog(LOG_INFO, "D3D11 video failed (ret=%d), trying OpenGL", vr);
        vi.graphics_module = kOpenGL;
        vr = obs_reset_video(&vi);
        if (vr != OBS_VIDEO_SUCCESS) {
            blog(LOG_ERROR, "OpenGL video also failed (ret=%d)", vr);
            obs_shutdown();
            emit errorOccurred(tr("Video system init failed"));
            return false;
        }
    }

    // 4. 初始化音频
    obs_audio_info ai = {};
    ai.samples_per_sec = sampleRate_;
    ai.speakers = (channels_ == 2) ? SPEAKERS_STEREO : SPEAKERS_MONO;
    if (!obs_reset_audio(&ai)) {
        blog(LOG_ERROR, "Audio init failed");
        obs_shutdown();
        emit errorOccurred(tr("Audio system init failed"));
        return false;
    }

    // 5. 创建默认场景
    scene_ = obs_scene_create("default");
    if (!scene_) {
        obs_shutdown();
        emit errorOccurred(tr("Scene creation failed"));
        return false;
    }
    obs_set_output_source(0, obs_scene_get_source(scene_));

    initialized_ = true;
    blog(LOG_INFO, "CaptureEngine initialized: %dx%d@%dfps, %dHz/%dch",
         width, height, fps, sampleRate_, channels_);
    return true;
}

void CaptureEngine::shutdown()
{
    if (!initialized_) return;

    // 先标记关机，阻止后续回调 emit 信号
    shuttingDown_.store(true, std::memory_order_release);

    stopAll();

    // 短暂等待，让正在执行的 OBS 回调完成
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scene_ = nullptr;
    obs_shutdown();
    initialized_ = false;
    blog(LOG_INFO, "CaptureEngine shutdown");
}

bool CaptureEngine::ready() const
{
    return initialized_;
}

// ──────────────────── 摄像头 ────────────────────

bool CaptureEngine::startVideo()
{
    if (!initialized_) return false;
    if (videoRunning()) return true;

    createCameraSource();
    if (!camSource_) return false;

    // 注册原始视频回调
    obs_add_raw_video_callback(nullptr, &CaptureEngine::onRawVideo, this);
    blog(LOG_INFO, "Video capture started");
    return true;
}

void CaptureEngine::stopVideo()
{
    obs_remove_raw_video_callback(&CaptureEngine::onRawVideo, this);

    if (camSource_) {
        obs_sceneitem_t *item = obs_scene_find_source(scene_, kCamSourceName);
        if (item) obs_sceneitem_remove(item);
        camSource_ = nullptr;
    }
    blog(LOG_INFO, "Video capture stopped");
}

bool CaptureEngine::videoRunning() const
{
    return camSource_ != nullptr;
}

// ──────────────────── 麦克风 ────────────────────

bool CaptureEngine::startAudio()
{
    if (!initialized_) return false;
    if (audioRunning()) return true;

    createMicSource();
    if (!micSource_) return false;

    // 注册音频回调（float 交叠格式）
    audio_convert_info conv = {};
    conv.format          = AUDIO_FORMAT_FLOAT;
    conv.samples_per_sec = sampleRate_;
    conv.speakers        = (channels_ == 2) ? SPEAKERS_STEREO : SPEAKERS_MONO;
    obs_add_raw_audio_callback(0, &conv, &CaptureEngine::onRawAudio, this);
    blog(LOG_INFO, "Audio capture started");
    return true;
}

void CaptureEngine::stopAudio()
{
    obs_remove_raw_audio_callback(0, &CaptureEngine::onRawAudio, this);

    if (micSource_) {
        obs_sceneitem_t *item = obs_scene_find_source(scene_, kMicSourceName);
        if (item) obs_sceneitem_remove(item);
        micSource_ = nullptr;
    }
    blog(LOG_INFO, "Audio capture stopped");
}

bool CaptureEngine::audioRunning() const
{
    return micSource_ != nullptr;
}

void CaptureEngine::stopAll()
{
    stopVideo();
    stopAudio();
    stopScreenShare();
}

// ──────────────────── 设备枚举 ────────────────────

void CaptureEngine::enumCameras(DeviceCb cb)
{
    obs_properties_t *props = obs_get_source_properties("dshow_input");
    if (!props) return;

    obs_property_t *prop = obs_properties_get(props, "video_device_id");
    if (!prop) { obs_properties_destroy(props); return; }

    size_t count = obs_property_list_item_count(prop);
    for (size_t i = 0; i < count; ++i) {
        const char *id   = obs_property_list_item_string(prop, i);
        const char *name = obs_property_list_item_name(prop, i);
        if (id && name && !cb(id, name)) break;
    }
    obs_properties_destroy(props);
}

void CaptureEngine::enumMics(DeviceCb cb)
{
    obs_properties_t *props = obs_get_source_properties("wasapi_input_capture");
    if (!props) return;

    obs_property_t *prop = obs_properties_get(props, "device_id");
    if (!prop) { obs_properties_destroy(props); return; }

    size_t count = obs_property_list_item_count(prop);
    for (size_t i = 0; i < count; ++i) {
        const char *id   = obs_property_list_item_string(prop, i);
        const char *name = obs_property_list_item_name(prop, i);
        if (id && name && !cb(id, name)) break;
    }
    obs_properties_destroy(props);
}

// ──────────────────── 源创建 ────────────────────

void CaptureEngine::createCameraSource()
{
    const char *deviceId = nullptr;

    if (!camDeviceId_.empty()) {
        deviceId = camDeviceId_.c_str();
    } else {
        // 未指定设备时选第一个
        enumCameras([&](const char *id, const char * /*name*/) {
            deviceId = id;
            return false;
        });
    }

    if (!deviceId) {
        emit errorOccurred(tr("No camera found"));
        return;
    }

    // 记住当前设备
    camDeviceId_ = deviceId;

    obs_data_t *settings = obs_data_create();
    obs_data_set_string(settings, "video_device_id", deviceId);

    camSource_ = obs_source_create("dshow_input", kCamSourceName, settings, nullptr);
    obs_data_release(settings);

    if (!camSource_) {
        emit errorOccurred(tr("Failed to create camera source"));
        return;
    }

    obs_sceneitem_t *item = obs_scene_add(scene_, camSource_);

    obs_video_info vi;
    if (item && obs_get_video_info(&vi)) {
        vec2 bounds;
        bounds.x = (float)vi.base_width;
        bounds.y = (float)vi.base_height;
        obs_sceneitem_set_bounds(item, &bounds);
        obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
        obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
    }

    blog(LOG_INFO, "Camera source created: %s", deviceId);
}

void CaptureEngine::createMicSource()
{
    const char *deviceId = nullptr;

    if (!micDeviceId_.empty()) {
        deviceId = micDeviceId_.c_str();
    } else {
        enumMics([&](const char *id, const char * /*name*/) {
            deviceId = id;
            return false;
        });
    }

    if (!deviceId) {
        emit errorOccurred(tr("No microphone found"));
        return;
    }

    micDeviceId_ = deviceId;

    obs_data_t *settings = obs_data_create();
    obs_data_set_string(settings, "device_id", deviceId);

    micSource_ = obs_source_create("wasapi_input_capture", kMicSourceName, settings, nullptr);
    obs_data_release(settings);

    if (!micSource_) {
        emit errorOccurred(tr("Failed to create mic source"));
        return;
    }

    obs_source_set_monitoring_type(micSource_, OBS_MONITORING_TYPE_NONE);
    obs_scene_add(scene_, micSource_);

    blog(LOG_INFO, "Mic source created: %s", deviceId);
}

void CaptureEngine::switchCamera(const char *deviceId)
{
    if (!deviceId || camDeviceId_ == deviceId) return;

    bool wasRunning = videoRunning();
    if (wasRunning)
        stopVideo();

    camDeviceId_ = deviceId;

    if (wasRunning)
        startVideo();

    blog(LOG_INFO, "Camera switched to: %s", deviceId);
}

void CaptureEngine::switchMic(const char *deviceId)
{
    if (!deviceId || micDeviceId_ == deviceId) return;

    bool wasRunning = audioRunning();
    if (wasRunning)
        stopAudio();

    micDeviceId_ = deviceId;

    if (wasRunning)
        startAudio();

    blog(LOG_INFO, "Mic switched to: %s", deviceId);
}

std::string CaptureEngine::currentCameraId() const
{
    return camDeviceId_;
}

std::string CaptureEngine::currentMicId() const
{
    return micDeviceId_;
}

void CaptureEngine::destroySource(const char *name)
{
    obs_sceneitem_t *item = obs_scene_find_source(scene_, name);
    if (item) obs_sceneitem_remove(item);
}

// ──────────────────── 屏幕共享 ────────────────────

bool CaptureEngine::startScreenShare()
{
    if (!initialized_) return false;
    if (isScreenSharing()) return true;

    createScreenSource();
    if (!screenSource_) {
        emit errorOccurred(tr("Failed to create screen capture source"));
        return false;
    }

    obs_sceneitem_t *item = obs_scene_add(scene_, screenSource_);

    // 充满整个画布
    if (item) {
        obs_video_info vi;
        if (obs_get_video_info(&vi)) {
            vec2 bounds;
            bounds.x = (float)vi.base_width;
            bounds.y = (float)vi.base_height;
            obs_sceneitem_set_bounds(item, &bounds);
            obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
            obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
        }
    }

    blog(LOG_INFO, "Screen share started");
    return true;
}

void CaptureEngine::stopScreenShare()
{
    if (!screenSource_) return;

    // 从场景中移除
    obs_sceneitem_t *item = obs_scene_find_source(scene_, kScreenSourceName);
    if (item) obs_sceneitem_remove(item);

    screenSource_ = nullptr;
    blog(LOG_INFO, "Screen share stopped");
}

bool CaptureEngine::isScreenSharing() const
{
    return screenSource_ != nullptr;
}

void CaptureEngine::createScreenSource()
{
    // 枚举显示器列表，选第一个
    obs_properties_t *props = obs_get_source_properties("monitor_capture");
    if (!props) {
        // 尝试 display_capture (不同 OBS 版本可能不同)
        props = obs_get_source_properties("display_capture");
    }
    if (!props) {
        emit errorOccurred(tr("Screen capture not supported"));
        return;
    }

    obs_property_t *prop = obs_properties_get(props, "monitor");
    if (!prop) prop = obs_properties_get(props, "monitor_id");
    if (!prop) {
        obs_properties_destroy(props);
        emit errorOccurred(tr("No monitor property found"));
        return;
    }

    size_t count = obs_property_list_item_count(prop);
    if (count == 0) {
        obs_properties_destroy(props);
        emit errorOccurred(tr("No monitors found"));
        return;
    }

    // 选第一个显示器
    const char *monitorId = obs_property_list_item_string(prop, 0);
    const char *monitorName = obs_property_list_item_name(prop, 0);

    obs_data_t *settings = obs_data_create();
    if (monitorId && monitorId[0]) {
        const char *key = obs_property_name(prop);
        obs_data_set_string(settings, key, monitorId);
    }

    // 先尝试 monitor_capture，回退 display_capture
    const char *sourceType = "monitor_capture";
    screenSource_ = obs_source_create(sourceType, kScreenSourceName, settings, nullptr);
    if (!screenSource_) {
        sourceType = "display_capture";
        screenSource_ = obs_source_create(sourceType, kScreenSourceName, settings, nullptr);
    }

    obs_data_release(settings);
    obs_properties_destroy(props);

    if (!screenSource_) {
        emit errorOccurred(tr("Failed to create screen source"));
        return;
    }

    blog(LOG_INFO, "Screen capture source created: %s", monitorName ? monitorName : "Monitor");
}

// ──────────────────── OBS 原始回调 ────────────────────

void CaptureEngine::onRawVideo(void *param, video_data *frame)
{
    auto *self = static_cast<CaptureEngine *>(param);
    if (!self || !frame) return;
    if (self->shuttingDown_.load(std::memory_order_acquire)) return;

    std::lock_guard<std::mutex> lock(self->callbackMutex_);
    VideoFrame vf = self->convertVideo(frame);
    if (vf.valid()) {
        emit self->videoFrameReady(vf);
    }
}

void CaptureEngine::onRawAudio(void *param, size_t /*mixIdx*/, audio_data *data)
{
    auto *self = static_cast<CaptureEngine *>(param);
    if (!self || !data) return;
    if (self->shuttingDown_.load(std::memory_order_acquire)) return;

    std::lock_guard<std::mutex> lock(self->callbackMutex_);
    AudioFrame af = self->convertAudio(data);
    if (af.valid()) {
        emit self->audioFrameReady(af);
    }
}

// ──────────────────── 帧转换 ────────────────────

VideoFrame CaptureEngine::convertVideo(const video_data *obsFrame)
{
    VideoFrame frame;

    obs_video_info vi;
    if (!obs_get_video_info(&vi)) return frame;

    frame.width  = vi.base_width;
    frame.height = vi.base_height;
    frame.timestampUs = obsFrame->timestamp / 1000;  // ns → us

    int ySize  = frame.ySize();
    int uvSize = frame.uvSize();

    // 为每个平面分配紧凑内存
    frame.data[0] = std::shared_ptr<uint8_t[]>(new uint8_t[ySize]);
    frame.data[1] = std::shared_ptr<uint8_t[]>(new uint8_t[uvSize]);
    frame.data[2] = std::shared_ptr<uint8_t[]>(new uint8_t[uvSize]);

    // Y 平面（逐行拷贝，剔除 stride 填充）
    for (int y = 0; y < frame.height; ++y)
        std::memcpy(frame.data[0].get() + y * frame.width,
                    obsFrame->data[0] + y * obsFrame->linesize[0],
                    frame.width);

    // U 平面
    for (int y = 0; y < frame.height / 2; ++y)
        std::memcpy(frame.data[1].get() + y * (frame.width / 2),
                    obsFrame->data[1] + y * obsFrame->linesize[1],
                    frame.width / 2);

    // V 平面
    for (int y = 0; y < frame.height / 2; ++y)
        std::memcpy(frame.data[2].get() + y * (frame.width / 2),
                    obsFrame->data[2] + y * obsFrame->linesize[2],
                    frame.width / 2);

    return frame;
}

AudioFrame CaptureEngine::convertAudio(const audio_data *obsData)
{
    AudioFrame frame;
    frame.samples    = (int)obsData->frames;
    frame.timestampUs = obsData->timestamp / 1000;  // ns → us

    obs_audio_info ai;
    if (obs_get_audio_info(&ai)) {
        frame.sampleRate = ai.samples_per_sec;
        frame.channels   = (ai.speakers == SPEAKERS_STEREO) ? 2 : 1;
    }

    if (obsData->frames > 0 && obsData->data[0]) {
        size_t size = obsData->frames * frame.channels * sizeof(float);
        frame.data = std::shared_ptr<uint8_t[]>(new uint8_t[size]);
        std::memcpy(frame.data.get(), obsData->data[0], size);
    }
    return frame;
}

} // namespace Capture
