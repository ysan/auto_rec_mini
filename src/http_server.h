#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <iostream>
#include <string>

#include "ThreadMgrIf.h"
#include "httplib.h"
#include "nlohmann/json.hpp"

#include "Utils.h"
#include "Settings.h"
#include "ViewingManagerIf.h"
#include "ChannelManagerIf.h"
#include "PsisiManagerIf.h"


class http_server {
public:
	http_server (int port, threadmgr::CThreadMgrExternalIf *ext_if) : m_port(port), mp_ext_if(ext_if) {
	}
	virtual ~http_server (void) = default;

	void up (void) {
		{
			m_server.Post("/api/ctrl/viewing/start_by_physical_channel", [&](const httplib::Request &req, httplib::Response &res) {
				if (req.get_header_value("Content-Type") != "application/json") {
					res.status = 400;
					return ;
				}
				if (req.body.empty()) {
					res.status = 400;
					return ;
				}
				const auto r = nlohmann::json::parse(req.body);
				if (!r.contains("physical_channel")) {
					res.status = 400;
					return ;
				}
				if (!r.contains("service_idx")) {
					res.status = 400;
					return ;
				}
				uint16_t phy_ch = r["physical_channel"].get<uint16_t>();
				int svc_idx = r["service_idx"].get<int>();

				bool has_option = false;
				int _auto_stop_grace_period_sec = 0;
				if (r.contains("option") && r["option"].is_object()) {
					has_option = true;
					const auto &opt = r["option"];
					if (opt.contains("auto_stop_grace_period_sec")) {
						_auto_stop_grace_period_sec = opt["auto_stop_grace_period_sec"].get<int>();
					}
				}

				mp_ext_if->create_external_cp();
				CViewingManagerIf::physical_channel_param_t param = {
					.physical_channel = phy_ch,
					.service_idx = svc_idx
				};
				CViewingManagerIf _if (mp_ext_if);
				_if.request_start_viewing_by_physical_channel(&param, false);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					uint8_t gr = *(reinterpret_cast<uint8_t*>(src.get_message().data()));
					nlohmann::json j;
					j["group_id"] = static_cast<int>(gr);
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());

					if (has_option) {
						CViewingManagerIf::option_t opt = {
							.group_id = gr,
							.auto_stop_grace_period_sec = _auto_stop_grace_period_sec,
						};
						_if.request_set_option(&opt, false);
						threadmgr::CSource &src = mp_ext_if->receive_external();
//TODO
//						threadmgr::result rslt = src.get_result();
//						if (rslt == threadmgr::result::success) {
//						} else {
//						}
					}

				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Post("/api/ctrl/viewing/start_by_service_id", [&](const httplib::Request &req, httplib::Response &res) {
				if (req.get_header_value("Content-Type") != "application/json") {
					res.status = 400;
					return ;
				}
				if (req.body.empty()) {
					res.status = 400;
					return ;
				}
				const auto r = nlohmann::json::parse(req.body);
				if (!r.contains("transport_stream_id")) {
					res.status = 400;
					return ;
				}
				if (!r.contains("original_network_id")) {
					res.status = 400;
					return ;
				}
				if (!r.contains("service_id")) {
					res.status = 400;
					return ;
				}
				uint16_t ts_id = r["transport_stream_id"].get<uint16_t>();
				uint16_t org_nid = r["original_network_id"].get<uint16_t>();
				uint16_t svc_id = r["service_id"].get<uint16_t>();

				bool has_option = false;
				int _auto_stop_grace_period_sec = 0;
				if (r.contains("option") && r["option"].is_object()) {
					has_option = true;
					const auto &opt = r["option"];
					if (opt.contains("auto_stop_grace_period_sec")) {
						_auto_stop_grace_period_sec = opt["auto_stop_grace_period_sec"].get<int>();
					}
				}

				mp_ext_if->create_external_cp();
				CViewingManagerIf::service_id_param_t param = {
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = svc_id
				};
				CViewingManagerIf _if (mp_ext_if);
				_if.request_start_viewing_by_service_id(&param, false);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					uint8_t gr = *(reinterpret_cast<uint8_t*>(src.get_message().data()));
					nlohmann::json j;
					j["group_id"] = static_cast<int>(gr);
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());

					if (has_option) {
						CViewingManagerIf::option_t opt = {
							.group_id = gr,
							.auto_stop_grace_period_sec = _auto_stop_grace_period_sec,
						};
						_if.request_set_option(&opt, false);
						threadmgr::CSource &src = mp_ext_if->receive_external();
//TODO
//						threadmgr::result rslt = src.get_result();
//						if (rslt == threadmgr::result::success) {
//						} else {
//						}
					}

				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Post("/api/ctrl/viewing/stop", [&](const httplib::Request &req, httplib::Response &res) {
				if (req.get_header_value("Content-Type") != "application/json") {
					res.status = 400;
					return ;
				}
				if (req.body.empty()) {
					res.status = 400;
					return ;
				}
				const auto r = nlohmann::json::parse(req.body);
				if (!r.contains("group_id")) {
					res.status = 400;
					return ;
				}
				uint8_t gr = r["group_id"].get<uint8_t>();

				mp_ext_if->create_external_cp();
				CViewingManagerIf _if (mp_ext_if);
				_if.request_stop_viewing(gr, false);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Post("/api/ctrl/viewing/auto_stop_grace_period", [&](const httplib::Request &req, httplib::Response &res) {
				if (req.get_header_value("Content-Type") != "application/json") {
					res.status = 400;
					return ;
				}
				if (req.body.empty()) {
					res.status = 400;
					return ;
				}
				const auto r = nlohmann::json::parse(req.body);
				if (!r.contains("group_id")) {
					res.status = 400;
					return ;
				}
				if (!r.contains("period_sec")) {
					res.status = 400;
					return ;
				}
				uint8_t gr = r["group_id"].get<uint8_t>();
				int period = r["period_sec"].get<int>();

				mp_ext_if->create_external_cp();
				CViewingManagerIf _if (mp_ext_if);
				CViewingManagerIf::option_t opt = {
					.group_id = gr,
					.auto_stop_grace_period_sec = period,
				};
				_if.request_set_option(&opt, false);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Get("/api/ctrl/channel/channels", [&](const httplib::Request &req, httplib::Response &res) {
				mp_ext_if->create_external_cp();
				CChannelManagerIf::channel_t channels[20] = {0};
				CChannelManagerIf::request_channels_param_t param = {
					.p_out_channels = channels,
					.array_max_num = 20,
				};
				CChannelManagerIf _if (mp_ext_if);
				_if.request_get_channels_sync(&param);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					nlohmann::json j = nlohmann::json::array();
					res.set_content(j.dump(), "application/json");
					int ch_num = *(reinterpret_cast<int*>(src.get_message().data()));
					if (ch_num > 0) {
						for (int i = 0; i < ch_num; ++ i) {
							nlohmann::json j_child;
							j_child["physical_channel"] = channels[i].physical_channel;
							j_child["transport_stream_id"] = channels[i].transport_stream_id;
							j_child["original_network_id"] = channels[i].original_network_id;
							j_child["remote_control_key_id"] = channels[i].remote_control_key_id;
							j_child["service_ids"] = nlohmann::json::array();
							for (int j = 0; j < channels[i].service_num; ++ j) {
								j_child["service_ids"].push_back(channels[i].service_ids[j]);
							}
							j.push_back(j_child);
						}
					}
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Get(R"(/api/ctrl/channel/original_network_name/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_org_nid = req.matches[1];

				uint16_t org_nid = std::atoi(_m_org_nid.str().c_str());

				mp_ext_if->create_external_cp();
				CChannelManagerIf::service_id_param_t param = {
					.transport_stream_id = 0,
					.original_network_id = org_nid,
					.service_id = 0
				};
				CChannelManagerIf _if (mp_ext_if);
				_if.request_get_original_network_name(&param);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					if (src.get_message().length() > 0) {
						nlohmann::json j;
						j["original_network_name"] = reinterpret_cast<char*>(src.get_message().data());
						res.set_content(j.dump(), "application/json");
						res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
					} else {
						res.status = 500;
					}
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Get(R"(/api/ctrl/channel/transport_stream_name/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_org_nid = req.matches[1];
				auto _m_ts_id = req.matches[2];

				uint16_t org_nid = std::atoi(_m_org_nid.str().c_str());
				uint16_t ts_id = std::atoi(_m_ts_id.str().c_str());

				mp_ext_if->create_external_cp();
				CChannelManagerIf::service_id_param_t param = {
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = 0
				};
				CChannelManagerIf _if (mp_ext_if);
				_if.request_get_transport_stream_name_sync(&param);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					if (src.get_message().length() > 0) {
						nlohmann::json j;
						j["transport_stream_name"] = reinterpret_cast<char*>(src.get_message().data());
						res.set_content(j.dump(), "application/json");
						res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
					} else {
						res.status = 500;
					}
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Get(R"(/api/ctrl/channel/service_name/(\d+)/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_org_nid = req.matches[1];
				auto _m_ts_id = req.matches[2];
				auto _m_svc_id = req.matches[3];

				uint16_t ts_id = std::atoi(_m_org_nid.str().c_str());
				uint16_t org_nid = std::atoi(_m_ts_id.str().c_str());
				uint16_t svc_id = std::atoi(_m_svc_id.str().c_str());

				mp_ext_if->create_external_cp();
				CChannelManagerIf::service_id_param_t param = {
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = svc_id
				};
				CChannelManagerIf _if (mp_ext_if);
				_if.request_get_service_name_sync(&param);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					if (src.get_message().length() > 0) {
						nlohmann::json j;
						j["service_name"] = reinterpret_cast<char*>(src.get_message().data());
						res.set_content(j.dump(), "application/json");
						res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
					} else {
						res.status = 500;
					}
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Get(R"(/api/ctrl/psisi/(\d+)/present_event/(\d+)/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_gr = req.matches[1];
				auto _m_org_nid = req.matches[2];
				auto _m_ts_id = req.matches[3];
				auto _m_svc_id = req.matches[4];

				uint8_t _gr = std::atoi(_m_gr.str().c_str());
				uint16_t ts_id = std::atoi(_m_org_nid.str().c_str());
				uint16_t org_nid = std::atoi(_m_ts_id.str().c_str());
				uint16_t svc_id = std::atoi(_m_svc_id.str().c_str());

				mp_ext_if->create_external_cp();
				CPsisiManagerIf _if (mp_ext_if, _gr);
				psisi_structs::service_info_t _svc_info = {
					.table_id = 0,
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = svc_id,
					.service_type = 0,
					.p_service_name_char = nullptr,
				};
				psisi_structs::event_info_t _event_info ;
				_event_info.clear();
				_if.request_get_present_event_info_sync (&_svc_info, &_event_info);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					nlohmann::json j;
					j["transport_stream_id"] = _event_info.transport_stream_id;
					j["original_network_id"] = _event_info.original_network_id;
					j["service_id"] = _event_info.service_id;
					j["event_id"] = _event_info.event_id;
					j["start_time"] = _event_info.start_time.m_time.tv_sec;
					j["end_time"] = _event_info.end_time.m_time.tv_sec;
					j["event_name_char"] = _event_info.event_name_char;
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});

			m_server.Get(R"(/api/ctrl/psisi/(\d+)/follow_event/(\d+)/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_gr = req.matches[1];
				auto _m_org_nid = req.matches[2];
				auto _m_ts_id = req.matches[3];
				auto _m_svc_id = req.matches[4];

				uint8_t _gr = std::atoi(_m_gr.str().c_str());
				uint16_t ts_id = std::atoi(_m_org_nid.str().c_str());
				uint16_t org_nid = std::atoi(_m_ts_id.str().c_str());
				uint16_t svc_id = std::atoi(_m_svc_id.str().c_str());

				mp_ext_if->create_external_cp();
				CPsisiManagerIf _if (mp_ext_if, _gr);
				psisi_structs::service_info_t _svc_info = {
					.table_id = 0,
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = svc_id,
					.service_type = 0,
					.p_service_name_char = nullptr,
				};
				psisi_structs::event_info_t _event_info ;
				_event_info.clear();
				_if.request_get_follow_event_info_sync (&_svc_info, &_event_info);
				threadmgr::CSource &src = mp_ext_if->receive_external();
				threadmgr::result rslt = src.get_result();
				if (rslt == threadmgr::result::success) {
					nlohmann::json j;
					j["transport_stream_id"] = _event_info.transport_stream_id;
					j["original_network_id"] = _event_info.original_network_id;
					j["service_id"] = _event_info.service_id;
					j["event_id"] = _event_info.event_id;
					j["start_time"] = _event_info.start_time.m_time.tv_sec;
					j["end_time"] = _event_info.end_time.m_time.tv_sec;
					j["event_name_char"] = _event_info.event_name_char;
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
				mp_ext_if->destroy_external_cp();
			});
		}

//		{
//			m_server.set_file_request_handler([](const httplib::Request &req, httplib::Response &res) {
//				res.set_header("Access-Control-Allow-Origin", "*");
//			});
//		}

		{
			std::string *static_path = CSettings::getInstance()->getParams()->getHttpServerStaticContentsPath();
			httplib::Headers headers;
			headers.emplace("Access-Control-Allow-Origin", "*");
			if (!m_server.set_mount_point("/", static_path->c_str(), headers)) {
				_UTL_LOG_E("set_mount_point [%s] failure.", static_path->c_str());
			}
		}
		{
			std::string *stream_path = CSettings::getInstance()->getParams()->getViewingStreamDataPath();
			httplib::Headers headers;
			headers.emplace("Access-Control-Allow-Origin", "*");
			if (!m_server.set_mount_point("/stream/", stream_path->c_str(), headers)) {
				_UTL_LOG_E("set_mount_point [%s] failure.", stream_path->c_str());
			}
		}

		{
			// for CORS (preflight request)
			m_server.Options("/.+", [](const httplib::Request &req, httplib::Response &res) {
				res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				res.set_header("Allow", "GET, POST, HEAD, OPTIONS");
				res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
				res.set_header("Access-Control-Allow-Methods", "OPTIONS, GET, POST, HEAD");
			});
		}

		if (!m_server.listen("0.0.0.0", m_port)) {
			_UTL_LOG_E("HTTP bind failure.");
		}
		_UTL_LOG_I ("HTTP started");
	}

	void down (void) {
		_UTL_LOG_I ("HTTP stop...");
		m_server.stop();
		_UTL_LOG_I ("HTTP stop completed");
	}

private:
	httplib::Server m_server;
	int m_port;
	threadmgr::CThreadMgrExternalIf *mp_ext_if;
};

#endif
