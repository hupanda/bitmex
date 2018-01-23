#include "stdafx.h"
#include <chrono>
#include <string>
#include "json.hpp"
#include <vector>
#include <sstream>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <string>
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;
using json = nlohmann::json;

class cache {
public:

	cache() {}

	void set_symbols(std::vector<std::string> symbols) {
		m_symbols = symbols;
	}

	std::vector<std::string> get_topics() {
		//full topic list: https://testnet.bitmex.com/app/wsAPI
		std::vector<std::string> topics = { "execution", "order", "margin", "position", "wallet" };
		for (std::string symbol : m_symbols) {
			topics.push_back("orderBookL2:" + symbol);
			topics.push_back("quote:" + symbol);
			topics.push_back("trade:" + symbol);
		}
		return topics;
	}

	void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
		json r = json::parse(msg->get_payload());
		if (r.count("table") > 0)
			process_responce(r);
	}

	void process_responce(json r) {	
		std::string action = r["action"];
		std::string table = r["table"];
		if (m_keys.count(table) == 0 && action == "partial") {
			std::vector<std::string> r_keys = r["keys"];
			m_keys[table] = r_keys;
			json temp_data;
			m_data[table] = temp_data;
		}

		std::vector<json> r_data = r["data"];
		for (json d : r_data) {
			process_data(d, action, table);
		}
		return;
	}
	
	json get_data() {
		return m_data;
	}

private:
	json m_keys;
	json m_data;
	std::vector<std::string> m_symbols;

	void process_data(json d, std::string action, std::string table)
	{
		std::string key = extract_key(d, table);
		if (action == "delete")
			m_data[table].erase(key);
		else 
			m_data[table][key] = d;
	}

	std::string extract_key(json d, std::string table) {
		std::string key = "";
		for (std::string temp_key : m_keys[table]) {
			std::ostringstream strs;
			strs << d[temp_key];
			std::string str = strs.str();
			key.append(str);
		}
		return key;
	}

};