#include "stdafx.h"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <string>
#include "json.hpp"
#include "cache.cpp"
#include <vector>
#include <boost/algorithm/string/join.hpp>
using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

class bitmexwebsocket {
public:
	typedef bitmexwebsocket type;
	typedef std::chrono::duration<int, std::micro> dur_type;

	bitmexwebsocket() {
		m_endpoint.set_access_channels(websocketpp::log::alevel::all);
		m_endpoint.set_error_channels(websocketpp::log::elevel::all);
		m_endpoint.init_asio();
		m_endpoint.set_socket_init_handler(bind(&type::on_socket_init, this, ::_1));
		m_endpoint.set_tls_init_handler(bind(&type::on_tls_init,this,::_1));
		m_endpoint.set_message_handler(bind(&type::on_message, this, ::_1, ::_2));
		m_endpoint.set_open_handler(bind(&type::on_open, this, ::_1));
		m_endpoint.set_close_handler(bind(&type::on_close, this, ::_1));
		m_endpoint.set_fail_handler(bind(&type::on_fail, this, ::_1));
	}

	void init(cache* c) {
		m_cache = c;
		m_topics = c->get_topics();
	}

	void start() {
		websocketpp::lib::error_code ec;
		client::connection_ptr con = m_endpoint.get_connection(m_uri, ec);

		if (ec) {
			m_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
			return;
		}

		m_endpoint.connect(con);
		m_start = std::chrono::high_resolution_clock::now();
		m_endpoint.run();
	}

	void on_socket_init(websocketpp::connection_hdl) {
		m_socket_init = std::chrono::high_resolution_clock::now();
	}

	context_ptr on_tls_init(websocketpp::connection_hdl) {
		m_tls_init = std::chrono::high_resolution_clock::now();
		context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);

		try {
			ctx->set_options(boost::asio::ssl::context::default_workarounds |
				boost::asio::ssl::context::no_sslv2 |
				boost::asio::ssl::context::no_sslv3 |
				boost::asio::ssl::context::single_dh_use);
		}
		catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		return ctx;
	}

	void on_fail(websocketpp::connection_hdl hdl) {
		client::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
		std::cout << "Fail handler" << std::endl;
		std::cout << con->get_state() << std::endl;
		std::cout << con->get_local_close_code() << std::endl;
		std::cout << con->get_local_close_reason() << std::endl;
		std::cout << con->get_remote_close_code() << std::endl;
		std::cout << con->get_remote_close_reason() << std::endl;
		std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
	}

	void on_open(websocketpp::connection_hdl hdl) {
		m_connection = hdl;
		m_open = std::chrono::high_resolution_clock::now();
		std::string nonce = get_nonce();
		std::string m = "GET/realtime" + nonce;
		uint8_t *hmac_digest = HMAC(EVP_sha256(), m_secret.c_str(), m_secret.length(), reinterpret_cast<const uint8_t *>(m.c_str()), m.length(), NULL, NULL);
		std::string signature = hex_str(hmac_digest, hmac_digest + SHA256_DIGEST_LENGTH);
		send("authKey", m_key + "\"," + nonce + ",\"" + signature);
		send("subscribe", boost::algorithm::join(m_topics, "\",\""));
	}

	void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
		std::cout << "Message received: " << msg->get_payload() << std::endl << std::endl;
		process_message(msg->get_payload());
	}

	void on_close(websocketpp::connection_hdl) {
		m_close = std::chrono::high_resolution_clock::now();
		std::cout << "Socket Init: " << std::chrono::duration_cast<dur_type>(m_socket_init - m_start).count() << std::endl;
		std::cout << "TLS Init: " << std::chrono::duration_cast<dur_type>(m_tls_init - m_start).count() << std::endl;
		std::cout << "Open: " << std::chrono::duration_cast<dur_type>(m_open - m_start).count() << std::endl;
		std::cout << "Message: " << std::chrono::duration_cast<dur_type>(m_message - m_start).count() << std::endl;
		std::cout << "Close: " << std::chrono::duration_cast<dur_type>(m_close - m_start).count() << std::endl;
	}

	void send(std::string op, std::string args) {
		std::string payload = "{\"op\": \"" + op + "\", \"args\": [\"" + args + "\"]}";
		m_endpoint.send(m_connection, payload, websocketpp::frame::opcode::text);
	}

private:
	cache *m_cache;
	client m_endpoint;
	std::string m_uri = "wss://testnet.bitmex.com/realtime";
	websocketpp::connection_hdl m_connection;
	std::string m_key = "UmF9MG5zBvRvl45SqE1fOvM7";
	std::string m_secret = "-9k_Jktbcn-Fa01-Aipv2F0APvLa__TT_zY8VMh855rzld4f";
	std::vector<std::string> m_topics;
	std::chrono::high_resolution_clock::time_point m_start;
	std::chrono::high_resolution_clock::time_point m_socket_init;
	std::chrono::high_resolution_clock::time_point m_tls_init;
	std::chrono::high_resolution_clock::time_point m_open;
	std::chrono::high_resolution_clock::time_point m_message;
	std::chrono::high_resolution_clock::time_point m_close;

	void process_message(std::string message)
	{
		json r = json::parse(message);
		if (r.count("table") > 0)
			m_cache->process_responce(r);
	}

	std::string get_nonce() {
		return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	}

	enum {
		upperhex,
		lowerhex
	};

	template <bool Caps = lowerhex, typename FwdIt>
	std::string hex_str(FwdIt first, FwdIt last) {
		static_assert(sizeof(typename std::iterator_traits<FwdIt>::value_type) == 1, "value_type must be 1 byte.");
		constexpr const char *bytemap = Caps ? "0123456789abcdef" : "0123456789ABCDEF";
		std::string result(std::distance(first, last) * 2, '0');
		auto pos = begin(result);
		while (first != last) {
			*pos++ = bytemap[*first >> 4 & 0xF];
			*pos++ = bytemap[*first++ & 0xF];
		}
		return result;
	}

};