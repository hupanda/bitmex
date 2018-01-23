# Overview
* This is a simple windows VC++ app to connect to websocket API of [bitmex.com](www.bitmex.com), an crypto currency exchange
* The websocket API is used to subscribe to price, trade or position update, more details in the [official documentation](https://testnet.bitmex.com/app/wsAPI)
* Dependency
  * boost x64 version 66
  * openSSL x64
  * [websocketpp](ttps://github.com/zaphoyd/websocketpp) by [Peter Thorson](https://github.com/zaphoyd/)
  * [json](https://github.com/nlohmann/json) by [Niels Lohmann](https://github.com/nlohmann)

# Pre-compile Setup
1. Download boost_1_66_0
2. Download websocketpp
3. Install OpenSSL x64
4. Run b2 address-model=64 under boost folder
5. Run bjam address-model=64 under boost folder
6. In VS, include below directories
* \OpenSSL\include
* \websocketpp-master
* \boost_1_64_0\boost_1_64_0
7. In VS, add below to the linker libary directory
* \OpenSSL\lib\VC\static
* \OpenSSL\lib
* \OpenSSL\lib\VC
* \boost_1_64_0\boost_1_64_0\stage\lib
8. In VS, add below files to linker depencies (OpenSSL lib)
* libcrypto.lib
* libssl.lib
9. Compile may fail with error of keyword SSL_R_SHORT_READ not found. We can modify source file of websocketpp following [the instructions](https://github.com/zaphoyd/websocketpp/commit/16d126ee61dfc901e75abc5573b704c72a8d1f24).