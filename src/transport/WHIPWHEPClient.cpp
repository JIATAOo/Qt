#include "WHIPWHEPClient.h"
#include "Logger.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <regex>

static const std::string sc_default_secret = "18d44630c086480783f62f33dbb7e09a";
static constexpr int sc_default_port = 2022;

namespace TRANSPORT
{    
    WHIPWHEPClient::WHIPWHEPClient()
    {
        m_networkManager = new QNetworkAccessManager(this);
    }
    
    WHIPWHEPClient::~WHIPWHEPClient()
    {
    }

    WHIPWHEPClient::HttpResponse WHIPWHEPClient::postRequest(
        const std::string& url,
        const std::map<std::string, std::string>& headers,
        const std::string& body)
    {
        HttpResponse result;
        QUrl qurl(QString::fromStdString(url));
        QNetworkRequest request(qurl);

        for (const auto& [key, value] : headers) {
            request.setRawHeader(QByteArray::fromStdString(key), QByteArray::fromStdString(value));
        }

        QEventLoop loop;
        QNetworkReply* reply = m_networkManager->post(request, QByteArray::fromStdString(body));
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        result.status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.body = reply->readAll().toStdString();

        // Parse response headers
        const auto& rawHeaders = reply->rawHeaderPairs();
        for (const auto& pair : rawHeaders) {
            result.headers[pair.first.toStdString()] = pair.second.toStdString();
        }

        reply->deleteLater();
        return result;
    }

    WHIPWHEPClient::HttpResponse WHIPWHEPClient::deleteRequest(
        const std::string& url,
        const std::map<std::string, std::string>& headers)
    {
        HttpResponse result;
        QUrl qurl(QString::fromStdString(url));
        QNetworkRequest request(qurl);

        for (const auto& [key, value] : headers) {
            request.setRawHeader(QByteArray::fromStdString(key), QByteArray::fromStdString(value));
        }

        QEventLoop loop;
        QNetworkReply* reply = m_networkManager->deleteResource(request);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        result.status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.body = reply->readAll().toStdString();

        const auto& rawHeaders = reply->rawHeaderPairs();
        for (const auto& pair : rawHeaders) {
            result.headers[pair.first.toStdString()] = pair.second.toStdString();
        }

        reply->deleteLater();
        return result;
    }
    
    std::optional<WHIPResponse> WHIPWHEPClient::Publish(const PublishConfig& config, const std::string& sdp_offer)
    {
        std::string endpoint_url = BuildPublishEndpoint(config);
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = "application/sdp";

        HttpResponse http_response = postRequest(endpoint_url, headers, sdp_offer);
        if (http_response.status_code >= 200 && http_response.status_code < 300)
        {
            WHIPResponse response;
            response.sdp = http_response.body;
            response.resource_url = ParseResourceURL(http_response.headers);
            return response;
        }
        else
        {
            LOG_ERROR_STREAM << "Publish failed, status_code: " << http_response.status_code << ", body: " << http_response.body;
            return std::nullopt;
        }
    }
    
    bool WHIPWHEPClient::Unpublish(const std::string& resource_url, const std::string& http_url)
    {
        std::map<std::string, std::string> headers;
        std::string dest_url = "http://" + http_url + ":" + std::to_string(sc_default_port) + resource_url;

        HttpResponse http_response = deleteRequest(dest_url, headers);
        if (http_response.status_code >= 200 && http_response.status_code < 300)
        {
            return true;
        }
        else
        {
            LOG_ERROR_STREAM << "Unpublish failed, status_code: " << http_response.status_code << ", body: " << http_response.body;
            return false;
        }
    }
    
    std::optional<WHIPResponse> WHIPWHEPClient::Subscribe(const SubscribeConfig& config, const std::string& sdp_offer)
    {
        std::string endpoint_url = BuildSubscribeEndpoint(config);

        std::map<std::string, std::string> headers;
        headers["Content-Type"] = "application/sdp";

        HttpResponse http_response = postRequest(endpoint_url, headers, sdp_offer);
        if (http_response.status_code >= 200 && http_response.status_code < 300)
        {
            WHIPResponse response;
            response.sdp = http_response.body;
            response.resource_url = ParseResourceURL(http_response.headers);
            return response;
        }
        else
        {
            LOG_ERROR_STREAM << "Subscribe failed, status_code: " << http_response.status_code << ", body: " << http_response.body;
            return std::nullopt;
        }
    }
    
    bool WHIPWHEPClient::Unsubscribe(const std::string& resource_url, const std::string& http_url)
    {
        std::string dest_url = "http://" + http_url + ":" + std::to_string(sc_default_port) + resource_url;
        std::map<std::string, std::string> headers;
        HttpResponse http_response = deleteRequest(dest_url, headers);

        if (http_response.status_code >= 200 && http_response.status_code < 300)
        {
            return true;
        }
        else
        {
            LOG_ERROR_STREAM << "Unsubscribe failed, status_code: " << http_response.status_code << ", body: " << http_response.body;
            return false;
        }
    }
    
    std::string WHIPWHEPClient::BuildPublishEndpoint(const PublishConfig& config)
    {
        std::string endpoint = "http://" + config.endpoint_url + ":" + std::to_string(sc_default_port);
        if (endpoint.back() == '/')
        {
            endpoint.pop_back();
        }
        endpoint += "/rtc/v1/whip/?app=live&stream=" + config.room_id + "_" + config.local_user_id;
        endpoint += "&secret=" + sc_default_secret;
        return endpoint;
    }
    
    std::string WHIPWHEPClient::BuildSubscribeEndpoint(const SubscribeConfig& config)
    {
        std::string endpoint = "http://" + config.endpoint_url + ":" + std::to_string(sc_default_port);
        if (endpoint.back() == '/')
        {
            endpoint.pop_back();
        }
        endpoint += "/rtc/v1/whep/";
        endpoint += "?app=live&stream=" + config.room_id + "_" + config.remote_user_id;
        return endpoint;
    }
    
    std::string WHIPWHEPClient::ParseResourceURL(const std::map<std::string, std::string>& headers)
    {
        auto location_it = headers.find("Location");
        if (location_it != headers.end())
        {
            return location_it->second;
        }
        
        auto link_it = headers.find("Link");
        if (link_it != headers.end())
        {
            std::regex link_regex(R"(<([^>]+)>)");
            std::smatch match;
            if (std::regex_search(link_it->second, match, link_regex))
            {
                return match[1].str();
            }
        }
        
        return "";
    }

} // namespace TRANSPORT
