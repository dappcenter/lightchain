// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "bitcoin-cli.h"
#include "bitcoin-cliapi.h"
#include "uint256.h"

std::string HelpMessageCli()
{
    const auto defaultBaseParams = CreateBaseChainParams(CBaseChainParams::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(CBaseChainParams::TESTNET);
    std::string strUsage;
    strUsage += HelpMessageGroup(_("Options:"));
    strUsage += HelpMessageOpt("-?", _("This help message"));
    strUsage += HelpMessageOpt("-conf=<file>", strprintf(_("Specify configuration file (default: %s)"), BITCOIN_CONF_FILENAME));
    strUsage += HelpMessageOpt("-datadir=<dir>", _("Specify data directory"));
    AppendParamsHelpMessages(strUsage);
    strUsage += HelpMessageOpt("-named", strprintf(_("Pass named instead of positional arguments (default: %s)"), DEFAULT_NAMED));
    strUsage += HelpMessageOpt("-rpcconnect=<ip>", strprintf(_("Send commands to node running on <ip> (default: %s)"), DEFAULT_RPCCONNECT));
    strUsage += HelpMessageOpt("-rpcport=<port>", strprintf(_("Connect to JSON-RPC on <port> (default: %u or testnet: %u)"), defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort()));
    strUsage += HelpMessageOpt("-rpcwait", _("Wait for RPC server to start"));
    strUsage += HelpMessageOpt("-rpcuser=<user>", _("Username for JSON-RPC connections"));
    strUsage += HelpMessageOpt("-rpcpassword=<pw>", _("Password for JSON-RPC connections"));
    strUsage += HelpMessageOpt("-rpcclienttimeout=<n>", strprintf(_("Timeout in seconds during HTTP requests, or 0 for no timeout. (default: %d)"), DEFAULT_HTTP_CLIENT_TIMEOUT));
    strUsage += HelpMessageOpt("-stdin", _("Read extra arguments from standard input, one per line until EOF/Ctrl-D (recommended for sensitive information such as passphrases)"));
    strUsage += HelpMessageOpt("-rpcwallet=<walletname>", _("Send RPC for non-default wallet on RPC server (argument is wallet filename in bitcoind directory, required if bitcoind/-Qt runs with multiple wallets)"));

    return strUsage;
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//

//
// Exception thrown on connection error.  This error is used to determine
// when to wait if -rpcwait is given.
//
//
// This function returns either one of EXIT_ codes when it's expected to stop the process or
// CONTINUE_EXECUTION when it's expected to continue further.
//

const char *http_errorstring(int code)
{
    switch(code) {
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    case EVREQ_HTTP_TIMEOUT:
        return "timeout reached";
    case EVREQ_HTTP_EOF:
        return "EOF reached";
    case EVREQ_HTTP_INVALID_HEADER:
        return "error while reading header, or invalid header";
    case EVREQ_HTTP_BUFFER_ERROR:
        return "error encountered while reading or writing";
    case EVREQ_HTTP_REQUEST_CANCEL:
        return "request was canceled";
    case EVREQ_HTTP_DATA_TOO_LONG:
        return "response body is larger than allowed";
#endif
    default:
        return "unknown";
    }
}

static void http_request_done(struct evhttp_request *req, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);

    if (req == nullptr) {
        /* If req is nullptr, it means an error occurred while connecting: the
         * error code will have been passed to http_error_cb.
         */
        reply->status = 0;
        return;
    }

    reply->status = evhttp_request_get_response_code(req);

    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    if (buf)
    {
        size_t size = evbuffer_get_length(buf);
        const char *data = (const char*)evbuffer_pullup(buf, size);
        if (data)
            reply->body = std::string(data, size);
        evbuffer_drain(buf, size);
    }
}

#if LIBEVENT_VERSION_NUMBER >= 0x02010300
static void http_error_cb(enum evhttp_request_error err, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);
    reply->error = err;
}
#endif

UniValue CallRPC(const std::string& strMethod, const UniValue& params)
{
    std::string host;
    // In preference order, we choose the following for the port:
    //     1. -rpcport
    //     2. port in -rpcconnect (ie following : in ipv4 or ]: in ipv6)
    //     3. default port for chain
    int port = BaseParams().RPCPort();
    SplitHostPort(gArgs.GetArg("-rpcconnect", DEFAULT_RPCCONNECT), port, host);
    port = gArgs.GetArg("-rpcport", port);

    // Obtain event base
    raii_event_base base = obtain_event_base();

    // Synchronously look up hostname
    raii_evhttp_connection evcon = obtain_evhttp_connection_base(base.get(), host, port);
    evhttp_connection_set_timeout(evcon.get(), gArgs.GetArg("-rpcclienttimeout", DEFAULT_HTTP_CLIENT_TIMEOUT));

    HTTPReply response;
    raii_evhttp_request req = obtain_evhttp_request(http_request_done, (void*)&response);
    if (req == nullptr)
        throw std::runtime_error("create http request failed");
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    evhttp_request_set_error_cb(req.get(), http_error_cb);
#endif

    // Get credentials
    std::string strRPCUserColonPass;
    if (gArgs.GetArg("-rpcpassword", "") == "") {
        // Try fall back to cookie-based authentication if no password is provided
        if (!GetAuthCookie(&strRPCUserColonPass)) {
            throw std::runtime_error(strprintf(
                _("Could not locate RPC credentials. No authentication cookie could be found, and no rpcpassword is set in the configuration file (%s)"),
                    GetConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME)).string().c_str()));

        }
    } else {
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }

    struct evkeyvalq* output_headers = evhttp_request_get_output_headers(req.get());
    assert(output_headers);
    evhttp_add_header(output_headers, "Host", host.c_str());
    evhttp_add_header(output_headers, "Connection", "close");
    evhttp_add_header(output_headers, "Authorization", (std::string("Basic ") + EncodeBase64(strRPCUserColonPass)).c_str());

    // Attach request data
    std::string strRequest = JSONRPCRequestObj(strMethod, params, 1).write() + "\n";
    struct evbuffer* output_buffer = evhttp_request_get_output_buffer(req.get());
    assert(output_buffer);
    evbuffer_add(output_buffer, strRequest.data(), strRequest.size());

    // check if we should use a special wallet endpoint
    std::string endpoint = "/";
    std::string walletName = gArgs.GetArg("-rpcwallet", "");
    if (!walletName.empty()) {
        char *encodedURI = evhttp_uriencode(walletName.c_str(), walletName.size(), false);
        if (encodedURI) {
            endpoint = "/wallet/"+ std::string(encodedURI);
            free(encodedURI);
        }
        else {
            throw CConnectionFailed("uri-encode failed");
        }
    }
    int r = evhttp_make_request(evcon.get(), req.get(), EVHTTP_REQ_POST, endpoint.c_str());
    req.release(); // ownership moved to evcon in above call
    if (r != 0) {
        throw CConnectionFailed("send http request failed");
    }

    event_base_dispatch(base.get());

    if (response.status == 0)
        throw CConnectionFailed(strprintf("couldn't connect to server: %s (code %d)\n(make sure server is running and you are connecting to the correct RPC port)", http_errorstring(response.error), response.error));
    else if (response.status == HTTP_UNAUTHORIZED)
        throw std::runtime_error("incorrect rpcuser or rpcpassword (authorization failed)");
    else if (response.status >= 400 && response.status != HTTP_BAD_REQUEST && response.status != HTTP_NOT_FOUND && response.status != HTTP_INTERNAL_SERVER_ERROR)
        throw std::runtime_error(strprintf("server returned HTTP error %d", response.status));
    else if (response.body.empty())
        throw std::runtime_error("no response from server");

    // Parse reply
    UniValue valReply(UniValue::VSTR);
    if (!valReply.read(response.body))
        throw std::runtime_error("couldn't parse reply from server");
    const UniValue& reply = valReply.get_obj();
    if (reply.empty())
        throw std::runtime_error("expected reply to have result, error and id properties");

    return reply;
}


static int AppInitRPC(const std::string& conffile, const std::string& datadir)
{
    if(datadir != ""){
        gArgs.SoftSetArg("-datadir", datadir);
    }

    if (!fs::is_directory(GetDataDir(false))) {
        fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "").c_str());
        return EXIT_FAILURE;
    }

    try {
        gArgs.ReadConfigFile(conffile);
    } catch (const std::exception& e) {
        fprintf(stderr,"Error reading configuration file: %s\n", e.what());
        return EXIT_FAILURE;
    }

    // Check for -testnet or -regtest parameter (BaseParams() calls are only valid after this clause)
    try {
        std::string a = ChainNameFromCommandLine();
        fprintf(stdout,"a=%s\n",a.c_str());
        SelectBaseParams(ChainNameFromCommandLine());
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }
    if (gArgs.GetBoolArg("-rpcssl", false))
    {
        fprintf(stderr, "Error: SSL mode for RPC (-rpcssl) is no longer supported.\n");
        return EXIT_FAILURE;
    }
    return CONTINUE_EXECUTION;
}

int CommandRPC(const std::string& strMethod, const std::vector<std::string>& args, const int iMaxTryTime, UniValue& errCode, UniValue& errMsg, UniValue& result, std::string& strPrint)
{
    int nRet = 0;
    try {
        int maxTryTime = 3;
    	if(maxTryTime < iMaxTryTime){
    	    maxTryTime = iMaxTryTime;
    	}

        UniValue params = RPCConvertValues(strMethod, args);
        do {
            try {
                const UniValue reply = CallRPC(strMethod, params);

                // Parse reply
                result = find_value(reply, "result");
                const UniValue& error  = find_value(reply, "error");

                if (!error.isNull()) {
                    // Error
                    int code = error["code"].get_int();
                    if (code == RPC_IN_WARMUP)
                        throw CConnectionFailed("server in warmup");
                    strPrint = "error: " + error.write();
                    nRet = abs(code);
                    if (error.isObject())
                    {
                        errCode = find_value(error, "code");
                        errMsg  = find_value(error, "message");
                        strPrint = errCode.isNull() ? "" : "error code: "+errCode.getValStr()+"\n";

                        if (errMsg.isStr())
                            strPrint += "error message:\n"+errMsg.get_str();

                        if (errCode.isNum() && errCode.get_int() == RPC_WALLET_NOT_SPECIFIED) {
                            strPrint += "\nTry adding \"-rpcwallet=<filename>\" option to bitcoin-cli command line.";
                        }
                    }
                } else {
                    // Result
                    if (result.isNull())
                        strPrint = "";
                    else if (result.isStr())
                        strPrint = result.get_str();
                    else
                        strPrint = result.write(2);
                }
                // Connection succeeded, no need to retry.
                break;
            }
            catch (const CConnectionFailed&) {
		MilliSleep(1000);
            }
        } while (maxTryTime-- >0);
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (const std::exception& e) {
        strPrint = std::string("error: ") + e.what();
        nRet = EXIT_FAILURE;
    }
    catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRPC()");
        throw;
    }
    return nRet;
}

int runCommand(const std::string& conf_file, const std::string& datadir, const std::string& strMethod, const std::vector<std::string>& args, const int maxTryTime, UniValue& errCode, UniValue& errMsg, UniValue& result, std::string& strPrint)
{
    SetupEnvironment();
    if (!SetupNetworking()) {
        strPrint ="Error: Initializing networking failed\n";
        errCode = EXIT_FAILURE;
        return EXIT_FAILURE;
    }

    try {
        int ret = AppInitRPC(conf_file, datadir);
        if (ret != CONTINUE_EXECUTION)
            return ret;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRPC()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInitRPC()");
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;
    try {
        ret = CommandRPC(strMethod, args, maxTryTime, errCode, errMsg, result, strPrint);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRPC()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRPC()");
    }
    return ret;
}


int createsubchain(const createsubchainRequest& req, commonResponse& res)
{
	const std::string& rpc_conf_file = req.info.rpc_conf_file;
	const std::string& datadir = req.info.datadir;
	const std::string& subchainowner = req.subchainowner;
	const std::string& subchainid = req.subchainid;
	const std::string& subchainname = req.subchainname;
	const std::string& subcoinname = req.subcoinname;
	const std::string& subchainseeds = req.subchainseeds;

	int& retErrCode = res.retErrCode;
	std::string&  retErrMsg = res.retErrMsg;
	std::string& retDebugInfo = res.retDebugInfo;
	std::string& retResult = res.retResult;

	std::vector<std::string> args;
	std::string strMethod = "createsubchaininfo";
	args.push_back(subchainowner);
	args.push_back(subchainid);
	args.push_back(subchainname);
	args.push_back(subcoinname);
	args.push_back(subchainseeds);
	UniValue errCode;
	UniValue errMsg;
	UniValue result;
	retErrCode = 0;
	retErrMsg = "";
	if (errCode.isNum()){
		retErrCode = errCode.get_int();
	}
	if (errMsg.isStr()){
		retErrMsg = errMsg.get_str();
	}

	int ret = runCommand(rpc_conf_file, datadir, strMethod, args, 10 /*maxTryTimes*/, errCode, errMsg, result, retDebugInfo);
        UniValue  uresult = find_value(result,"data");
        if(uresult.type() == UniValue::VSTR){
		retResult = uresult.get_str(); 
        }
        return ret;
}

int gensubchainblock(const gensubchainblockRequest& req, commonResponse& res)
{
std::cout<<"hh1"<<std::endl;
	const std::string& rpc_conf_file = req.info.rpc_conf_file;
	const std::string& datadir = req.info.datadir;
	const std::string& subchainid = req.subchainid;
	const std::string&  subblockheight = req.subblockheight;
	const std::string&  subblockdata = req.subblockdata;
std::cout<<"hh2"<<std::endl;

	int& retErrCode = res.retErrCode;
	std::string&  retErrMsg = res.retErrMsg;
	std::string& retDebugInfo = res.retDebugInfo;
	std::string& retResult = res.retResult;

std::cout<<"hh3"<<std::endl;
	std::vector<std::string> args;
	std::string strMethod = "gensubchainblock";
	args.push_back(subchainid);
	args.push_back(subblockheight);
	args.push_back(subblockdata);
	UniValue errCode;
	UniValue errMsg;
	UniValue result;
	retErrCode = 0;
	retErrMsg = "";
	if (errCode.isNum()){
		retErrCode = errCode.get_int();
	}
	if (errMsg.isStr()){
		retErrMsg = errMsg.get_str();
	}
std::cout<<"hh4"<<std::endl;
	int ret = runCommand(rpc_conf_file, datadir, strMethod, args, 10 /*maxTryTimes*/, errCode, errMsg, result, retDebugInfo);
        UniValue  uresult = find_value(result,"result");
        if(uresult.type() == UniValue::VSTR){
		retResult = uresult.get_str(); 
        }
std::cout<<"hh5"<<std::endl;
	return ret;
}



int commitsubchain(const commitsubchainRequest& req, commonResponse& res)
{
	const std::string& rpc_conf_file = req.info.rpc_conf_file; 
	const std::string& datadir = req.info.datadir; 
	const std::string& wtxdata = req.wtxdata;
	const std::string& subblockdata = req.subblockdata;


	int& retErrCode = res.retErrCode;
	std::string&  retErrMsg = res.retErrMsg;
	std::string& retDebugInfo = res.retDebugInfo;

	std::vector<std::string> args;
	std::string strMethod = "commitsubchaininfo";
	args.push_back(wtxdata);
	args.push_back(subblockdata);

	UniValue errCode;
	UniValue errMsg;
	UniValue result;

	retErrCode = 0;
	retErrMsg = "";

	if (errCode.isNum()){
		retErrCode = errCode.get_int();
	}
	if (errMsg.isStr()){
		retErrMsg = errMsg.get_str();
	}
	return  runCommand(rpc_conf_file, datadir, strMethod, args, 10 /*maxTryTimes*/, errCode, errMsg, result, retDebugInfo);
}

int genbridgeownerinfo(const genbridgeownerinfoRequest& req, commonResponse& res)
{
	const std::string& rpc_conf_file = req.info.rpc_conf_file; 
	const std::string& datadir = req.info.datadir; 

	int& retErrCode = res.retErrCode;
	std::string&  retErrMsg = res.retErrMsg;
	std::string& retDebugInfo = res.retDebugInfo;

	std::vector<std::string> args;
	args.push_back(req.reset);
	std::string strMethod = "genbridgeowner";

	UniValue errCode;
	UniValue errMsg;
	UniValue result;

	retErrCode = 0;
	retErrMsg = "";

	if (errCode.isNum()){
		retErrCode = errCode.get_int();
	}
	if (errMsg.isStr()){
		retErrMsg = errMsg.get_str();
	}
	return  runCommand(rpc_conf_file, datadir, strMethod, args, 10 /*maxTryTimes*/, errCode, errMsg, result, retDebugInfo);
}