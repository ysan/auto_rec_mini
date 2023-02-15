#include <iostream>
#include <string>

#include "httplib.h"

#include "Utils.h"
#include "Settings.h"


class http_server {
public:
	http_server (int port=80) : m_port(port) {
	}
	virtual ~http_server (void) = default;

	void up (void) {
		{
			m_server.Post("/api/ctrl", [&](const httplib::Request &req, httplib::Response &res) {
std::cout << "reqbody[" << req.body << "]" << std::endl;

				res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());

				std::string body = "test";
				res.set_content(body, "application/json");
std::cout << "resbody[" << body << "]" << std::endl;
			});
		}
		{
			// for CORS (preflight request)
			m_server.Options("/api/ctrl", [](const httplib::Request &req, httplib::Response &res) {
				res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				res.set_header("Allow", "GET, POST, HEAD, OPTIONS");
				res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
				res.set_header("Access-Control-Allow-Methods", "OPTIONS, GET, POST, HEAD");
			});
		}

		{
			std::string *static_path = CSettings::getInstance()->getParams()->getHttpServerStaticContentsPath();
			if (!m_server.set_mount_point("/", static_path->c_str())) {
				_UTL_LOG_E("set_mount_point [%s] failure.", static_path->c_str());
			}
		}
		{
			std::string *stream_path = CSettings::getInstance()->getParams()->getViewingStreamDataPath();
			if (!m_server.set_mount_point("/stream/", stream_path->c_str())) {
				_UTL_LOG_E("set_mount_point [%s] failure.", stream_path->c_str());
			}
		}

		if (!m_server.listen("0.0.0.0", m_port)) {
			_UTL_LOG_E("HTTP bind failure.");
		}
	}

	void down (void) {
		m_server.stop();
	}

private:
	httplib::Server m_server;
	int m_port;
};
