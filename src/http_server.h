#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <iostream>
#include <string>

#include "coseqpp.h"
#include "httplib.h"
#include "nlohmann/json.hpp"

#include "utils.h"
#include "settings.h"
#include "viewing_manager_if.h"
#include "channel_manager_if.h"
#include "psisi_manager_if.h"


class http_server {
public:
	http_server (int port, coseq::manager *mgr) : port_(port), mgr_(mgr) {
	}
	virtual ~http_server (void) = default;

	void up (void) {
		{
			server_.Post("/api/ctrl/viewing/start_by_physical_channel", [&](const httplib::Request &req, httplib::Response &res) {
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

				arec::viewing::client_iface::physical_channel_param_t param = {
					.physical_channel = phy_ch,
					.service_idx = svc_idx
				};
				arec::viewing::client_iface _if (mgr_);
				arec::client_reply src = _if.request_start_viewing_by_physical_channel(&param, false);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
					uint8_t gr = *(reinterpret_cast<uint8_t*>(src.get_message().data()));
					nlohmann::json j;
					j["group_id"] = static_cast<int>(gr);
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());

					if (has_option) {
						arec::viewing::client_iface::option_t opt = {
							.group_id = gr,
							.auto_stop_grace_period_sec = _auto_stop_grace_period_sec,
						};
						arec::client_reply src = _if.request_set_option(&opt, false);
//TODO
//						coseq::result rslt = src.get_result();
//						if (rslt == coseq::result::success) {
//						} else {
//						}
					}

				} else {
					res.status = 500;
				}
			});

			server_.Post("/api/ctrl/viewing/start_by_service_id", [&](const httplib::Request &req, httplib::Response &res) {
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

				arec::viewing::client_iface::service_id_param_t param = {
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = svc_id
				};
				arec::viewing::client_iface _if (mgr_);
				arec::client_reply src = _if.request_start_viewing_by_service_id(&param, false);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
					uint8_t gr = *(reinterpret_cast<uint8_t*>(src.get_message().data()));
					nlohmann::json j;
					j["group_id"] = static_cast<int>(gr);
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());

					if (has_option) {
						arec::viewing::client_iface::option_t opt = {
							.group_id = gr,
							.auto_stop_grace_period_sec = _auto_stop_grace_period_sec,
						};
						arec::client_reply src = _if.request_set_option(&opt, false);
//TODO
//						coseq::result rslt = src.get_result();
//						if (rslt == coseq::result::success) {
//						} else {
//						}
					}

				} else {
					res.status = 500;
				}
			});

			server_.Post("/api/ctrl/viewing/stop", [&](const httplib::Request &req, httplib::Response &res) {
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

				arec::viewing::client_iface _if (mgr_);
				arec::client_reply src = _if.request_stop_viewing(gr, false);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
			});

			server_.Post("/api/ctrl/viewing/auto_stop_grace_period", [&](const httplib::Request &req, httplib::Response &res) {
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

				arec::viewing::client_iface _if (mgr_);
				arec::viewing::client_iface::option_t opt = {
					.group_id = gr,
					.auto_stop_grace_period_sec = period,
				};
				arec::client_reply src = _if.request_set_option(&opt, false);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
			});

			server_.Get("/api/ctrl/channel/channels", [&](const httplib::Request &req, httplib::Response &res) {
				arec::channel::client_iface::channel_t channels[20] = {0};
				arec::channel::client_iface::request_channels_param_t param = {
					.p_out_channels = channels,
					.array_max_num = 20,
				};
				arec::channel::client_iface _if (mgr_);
				arec::client_reply src = _if.request_get_channels_sync(&param);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
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
			});

			server_.Get(R"(/api/ctrl/channel/original_network_name/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_org_nid = req.matches[1];

				uint16_t org_nid = std::atoi(_m_org_nid.str().c_str());

				arec::channel::client_iface::service_id_param_t param = {
					.transport_stream_id = 0,
					.original_network_id = org_nid,
					.service_id = 0
				};
				arec::channel::client_iface _if (mgr_);
				arec::client_reply src = _if.request_get_original_network_name(&param);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
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
			});

			server_.Get(R"(/api/ctrl/channel/transport_stream_name/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_org_nid = req.matches[1];
				auto _m_ts_id = req.matches[2];

				uint16_t org_nid = std::atoi(_m_org_nid.str().c_str());
				uint16_t ts_id = std::atoi(_m_ts_id.str().c_str());

				arec::channel::client_iface::service_id_param_t param = {
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = 0
				};
				arec::channel::client_iface _if (mgr_);
				arec::client_reply src = _if.request_get_transport_stream_name_sync(&param);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
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
			});

			server_.Get(R"(/api/ctrl/channel/service_name/(\d+)/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_org_nid = req.matches[1];
				auto _m_ts_id = req.matches[2];
				auto _m_svc_id = req.matches[3];

				uint16_t ts_id = std::atoi(_m_org_nid.str().c_str());
				uint16_t org_nid = std::atoi(_m_ts_id.str().c_str());
				uint16_t svc_id = std::atoi(_m_svc_id.str().c_str());

				arec::channel::client_iface::service_id_param_t param = {
					.transport_stream_id = ts_id,
					.original_network_id = org_nid,
					.service_id = svc_id
				};
				arec::channel::client_iface _if (mgr_);
				arec::client_reply src = _if.request_get_service_name_sync(&param);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
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
			});

			server_.Get(R"(/api/ctrl/psisi/(\d+)/present_event/(\d+)/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_gr = req.matches[1];
				auto _m_org_nid = req.matches[2];
				auto _m_ts_id = req.matches[3];
				auto _m_svc_id = req.matches[4];

				uint8_t _gr = std::atoi(_m_gr.str().c_str());
				uint16_t ts_id = std::atoi(_m_org_nid.str().c_str());
				uint16_t org_nid = std::atoi(_m_ts_id.str().c_str());
				uint16_t svc_id = std::atoi(_m_svc_id.str().c_str());

				arec::psisi::client_iface _if (mgr_, _gr);
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
				arec::client_reply src = _if.request_get_present_event_info_sync(&_svc_info, &_event_info);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
					nlohmann::json j;
					j["transport_stream_id"] = _event_info.transport_stream_id;
					j["original_network_id"] = _event_info.original_network_id;
					j["service_id"] = _event_info.service_id;
					j["event_id"] = _event_info.event_id;
					j["start_time"] = _event_info.start_time.time_.tv_sec;
					j["end_time"] = _event_info.end_time.time_.tv_sec;
					j["event_name_char"] = _event_info.event_name_char;
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
			});

			server_.Get(R"(/api/ctrl/psisi/(\d+)/follow_event/(\d+)/(\d+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
				auto _m_gr = req.matches[1];
				auto _m_org_nid = req.matches[2];
				auto _m_ts_id = req.matches[3];
				auto _m_svc_id = req.matches[4];

				uint8_t _gr = std::atoi(_m_gr.str().c_str());
				uint16_t ts_id = std::atoi(_m_org_nid.str().c_str());
				uint16_t org_nid = std::atoi(_m_ts_id.str().c_str());
				uint16_t svc_id = std::atoi(_m_svc_id.str().c_str());

				arec::psisi::client_iface _if (mgr_, _gr);
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
				arec::client_reply src = _if.request_get_follow_event_info_sync(&_svc_info, &_event_info);
				coseq::result rslt = src.get_result();
				if (rslt == coseq::result::success) {
					nlohmann::json j;
					j["transport_stream_id"] = _event_info.transport_stream_id;
					j["original_network_id"] = _event_info.original_network_id;
					j["service_id"] = _event_info.service_id;
					j["event_id"] = _event_info.event_id;
					j["start_time"] = _event_info.start_time.time_.tv_sec;
					j["end_time"] = _event_info.end_time.time_.tv_sec;
					j["event_name_char"] = _event_info.event_name_char;
					res.set_content(j.dump(), "application/json");
					res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				} else {
					res.status = 500;
				}
			});
		}

//		{
//			server_.set_file_request_handler([](const httplib::Request &req, httplib::Response &res) {
//				res.set_header("Access-Control-Allow-Origin", "*");
//			});
//		}

		{
			std::string static_path = arec::settings::get_instance()->get_params().get_http_server_static_contents_path();
			httplib::Headers headers;
			headers.emplace("Access-Control-Allow-Origin", "*");
			if (!server_.set_mount_point("/", static_path.c_str(), headers)) {
				_UTL_LOG_E("set_mount_point [%s] failure.", static_path.c_str());
			}
		}
		{
			std::string stream_path = arec::settings::get_instance()->get_params().get_viewing_stream_data_path();
			httplib::Headers headers;
			headers.emplace("Access-Control-Allow-Origin", "*");
			if (!server_.set_mount_point("/stream/", stream_path.c_str(), headers)) {
				_UTL_LOG_E("set_mount_point [%s] failure.", stream_path.c_str());
			}
		}

		{
			// for CORS (preflight request)
			server_.Options("/.+", [](const httplib::Request &req, httplib::Response &res) {
				res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin").c_str());
				res.set_header("Allow", "GET, POST, HEAD, OPTIONS");
				res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
				res.set_header("Access-Control-Allow-Methods", "OPTIONS, GET, POST, HEAD");
			});
		}

		if (!server_.listen("0.0.0.0", port_)) {
			_UTL_LOG_E("HTTP bind failure.");
		}
		_UTL_LOG_I ("HTTP started");
	}

	void down (void) {
		_UTL_LOG_I ("HTTP stop...");
		server_.stop();
		_UTL_LOG_I ("HTTP stop completed");
	}

private:
	httplib::Server server_;
	int port_;
	coseq::manager *mgr_;
};

#endif
