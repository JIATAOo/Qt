#ifndef __WHIP_WHEP_CLIENT_H__
#define __WHIP_WHEP_CLIENT_H__

#include <QObject>
#include <QNetworkAccessManager>
#include <string>
#include <functional>
#include <memory>
#include <map>
#include <optional>

namespace TRANSPORT
{        
    
    struct BaseConfig
    {
        std::string endpoint_url;
        std::string room_id;
    };
    
    struct PublishConfig : BaseConfig
    {
        std::string local_user_id;
    };
    
    struct SubscribeConfig : BaseConfig
    {
        std::string remote_user_id;
    };
    
    struct WHIPResponse
    {
        std::string sdp;
        std::string resource_url;
    };
    
    class WHIPWHEPClient : public QObject
    {
        Q_OBJECT
    public:
        WHIPWHEPClient();
        ~WHIPWHEPClient();
        
        std::optional<WHIPResponse> Publish(const PublishConfig& config, const std::string& sdp_offer);
        bool Unpublish(const std::string& resource_url, const std::string& http_url);
        std::optional<WHIPResponse> Subscribe(const SubscribeConfig& config, const std::string& sdp_offer);
        bool Unsubscribe(const std::string& resource_url, const std::string& http_url);

private:     
        static std::string BuildPublishEndpoint(const PublishConfig& config);
        static std::string BuildSubscribeEndpoint(const SubscribeConfig& config);
        static std::string ParseResourceURL(const std::map<std::string, std::string>& headers);

        // Qt HTTP 同步请求辅助
        struct HttpResponse {
            int status_code = 0;
            std::string body;
            std::map<std::string, std::string> headers;
        };
        HttpResponse postRequest(const std::string& url, const std::map<std::string, std::string>& headers,
                                 const std::string& body);
        HttpResponse deleteRequest(const std::string& url, const std::map<std::string, std::string>& headers);

    private:
        QNetworkAccessManager* m_networkManager = nullptr;
    };
    
} // namespace TRANSPORT

#endif // __WHIP_WHEP_CLIENT_H__
