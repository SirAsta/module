#pragma once
#include "lua.h"
#include "lualib.h"
#include <Windows.h>
#include <winhttp.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

#pragma comment(lib, "winhttp.lib")

namespace http
{
    inline std::wstring to_wstring(const std::string& str)
    {
        if (str.empty()) return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
        std::wstring result(static_cast<size_t>(size), 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], size);
        return result;
    }

    inline std::string to_string(const std::wstring& wstr)
    {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string result(static_cast<size_t>(size), 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &result[0], size, nullptr, nullptr);
        return result;
    }

    inline const char* get_status_phrase(DWORD code)
    {
        switch (code)
        {
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
        }
    }

    struct Response
    {
        bool success = false;
        DWORD status_code = 0;
        std::string status_message;
        std::map<std::string, std::string> headers;
        std::vector<std::string> cookies;
        std::string body;
    };

    inline HINTERNET get_global_session()
    {
        static HINTERNET hSession = nullptr;
        if (!hSession)
        {
            hSession = WinHttpOpen(L"SkidBase/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (hSession)
            {
                DWORD decompression = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
                WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION, &decompression, sizeof(decompression));
                DWORD maxConns = 32;
                WinHttpSetOption(hSession, WINHTTP_OPTION_MAX_CONNS_PER_SERVER, &maxConns, sizeof(maxConns));
            }
        }
        return hSession;
    }

    inline Response perform_request(
        const std::string& url,
        const std::string& method,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& cookies,
        const std::string& body)
    {
        Response res;

        URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
        urlComp.dwHostNameLength = static_cast<DWORD>(-1);
        urlComp.dwUrlPathLength = static_cast<DWORD>(-1);
        urlComp.dwExtraInfoLength = static_cast<DWORD>(-1);

        std::wstring wurl = to_wstring(url);
        if (!WinHttpCrackUrl(wurl.c_str(), static_cast<DWORD>(wurl.length()), 0, &urlComp))
            return res;

        std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
        std::wstring path = (urlComp.dwUrlPathLength > 0) ? std::wstring(urlComp.lpszUrlPath, urlComp.dwUrlPathLength) : L"/";
        if (urlComp.dwExtraInfoLength > 0)
            path += std::wstring(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);

        bool is_https = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);
        INTERNET_PORT port = urlComp.nPort;

        HINTERNET hSession = get_global_session();
        if (!hSession) return res;

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
        if (!hConnect)
            return res;

        WinHttpSetTimeouts(hConnect, 3000, 3000, 5000, 5000);

        std::wstring wmethod = to_wstring(method);
        DWORD flags = is_https ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, wmethod.c_str(), path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            return res;
        }

        std::wstring header_str;
        for (const auto& [k, v] : headers)
            header_str += to_wstring(k) + L": " + to_wstring(v) + L"\r\n";

        if (!cookies.empty())
        {
            header_str += L"Cookie: ";
            bool first = true;
            for (const auto& [k, v] : cookies)
            {
                if (!first) header_str += L"; ";
                header_str += to_wstring(k) + L"=" + to_wstring(v);
                first = false;
            }
            header_str += L"\r\n";
        }

        DWORD total_hdr_len = static_cast<DWORD>(header_str.length());
        const void* body_data = body.empty() ? WINHTTP_NO_REQUEST_DATA : body.data();
        DWORD body_len = static_cast<DWORD>(body.length());

        if (WinHttpSendRequest(hRequest, header_str.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : header_str.c_str(), total_hdr_len, const_cast<void*>(body_data), body_len, body_len, 0))
        {
            if (WinHttpReceiveResponse(hRequest, nullptr))
            {
                res.success = true;

                DWORD status_code = 0;
                DWORD status_size = sizeof(status_code);
                WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_size, WINHTTP_NO_HEADER_INDEX);
                res.status_code = status_code;
                res.status_message = get_status_phrase(status_code);

                DWORD raw_header_size = 0;
                WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &raw_header_size, WINHTTP_NO_HEADER_INDEX);
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && raw_header_size > 0)
                {
                    std::vector<wchar_t> raw_headers(raw_header_size / sizeof(wchar_t) + 1, 0);
                    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, raw_headers.data(), &raw_header_size, WINHTTP_NO_HEADER_INDEX))
                    {
                        std::wstring all_hdrs = raw_headers.data();
                        size_t pos = 0;
                        while ((pos = all_hdrs.find(L"\r\n")) != std::wstring::npos)
                        {
                            std::wstring line = all_hdrs.substr(0, pos);
                            all_hdrs.erase(0, pos + 2);
                            size_t colon = line.find(L':');
                            if (colon != std::wstring::npos)
                            {
                                std::string k = to_string(line.substr(0, colon));
                                std::string v = to_string(line.substr(colon + 1));
                                while (!v.empty() && v.front() == ' ') v.erase(v.begin());
                                if (_stricmp(k.c_str(), "Set-Cookie") == 0)
                                    res.cookies.push_back(v);
                                res.headers[k] = v;
                            }
                        }
                    }
                }

                DWORD bytes_available = 0;
                while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available > 0)
                {
                    std::vector<char> buffer(bytes_available + 1, 0);
                    DWORD bytes_read = 0;
                    if (WinHttpReadData(hRequest, buffer.data(), bytes_available, &bytes_read) && bytes_read > 0)
                    {
                        res.body.append(buffer.data(), bytes_read);
                    }
                }
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return res;
    }

    inline int request(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);

        lua_getfield(L, 1, "Url");
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_getfield(L, 1, "url");
        }
        const char* raw_url = luaL_checkstring(L, -1);
        std::string url = raw_url ? raw_url : "";
        lua_pop(L, 1);

        std::string method = "GET";
        lua_getfield(L, 1, "Method");
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_getfield(L, 1, "method");
        }
        if (lua_isstring(L, -1))
            method = lua_tostring(L, -1);
        lua_pop(L, 1);
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);

        std::map<std::string, std::string> headers;
        lua_getfield(L, 1, "Headers");
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_getfield(L, 1, "headers");
        }
        if (lua_istable(L, -1))
        {
            lua_pushnil(L);
            while (lua_next(L, -2))
            {
                if (lua_isstring(L, -2) && lua_isstring(L, -1))
                    headers[lua_tostring(L, -2)] = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        std::map<std::string, std::string> cookies;
        lua_getfield(L, 1, "Cookies");
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_getfield(L, 1, "cookies");
        }
        if (lua_istable(L, -1))
        {
            lua_pushnil(L);
            while (lua_next(L, -2))
            {
                if (lua_isstring(L, -2) && lua_isstring(L, -1))
                    cookies[lua_tostring(L, -2)] = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        std::string body;
        lua_getfield(L, 1, "Body");
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_getfield(L, 1, "body");
        }
        if (lua_isstring(L, -1))
        {
            size_t blen = 0;
            const char* bstr = lua_tolstring(L, -1, &blen);
            if (bstr) body.assign(bstr, blen);
        }
        lua_pop(L, 1);

        Response res = perform_request(url, method, headers, cookies, body);

        lua_newtable(L);

        lua_pushboolean(L, res.success);
        lua_setfield(L, -2, "Success");

        lua_pushinteger(L, res.status_code);
        lua_setfield(L, -2, "StatusCode");

        lua_pushstring(L, res.status_message.c_str());
        lua_setfield(L, -2, "StatusMessage");

        lua_newtable(L);
        for (const auto& [k, v] : res.headers)
        {
            lua_pushstring(L, v.c_str());
            lua_setfield(L, -2, k.c_str());
        }
        lua_setfield(L, -2, "Headers");

        lua_newtable(L);
        for (size_t i = 0; i < res.cookies.size(); ++i)
        {
            lua_pushstring(L, res.cookies[i].c_str());
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        lua_setfield(L, -2, "Cookies");

        lua_pushlstring(L, res.body.data(), res.body.size());
        lua_setfield(L, -2, "Body");

        return 1;
    }

    inline int HttpGet(lua_State* L)
    {
        int idx = (lua_type(L, 1) == LUA_TUSERDATA || lua_type(L, 1) == LUA_TTABLE) ? 2 : 1;
        const char* raw_url = luaL_checkstring(L, idx);
        std::string url = raw_url ? raw_url : "";

        if (url.find("http://") != 0 && url.find("https://") != 0)
        {
            luaL_error(L, "Invalid protocol (expected 'http://' or 'https://')");
            return 0;
        }

        std::map<std::string, std::string> headers;
        if (lua_istable(L, idx + 1))
        {
            lua_pushnil(L);
            while (lua_next(L, idx + 1))
            {
                if (lua_isstring(L, -2) && lua_isstring(L, -1))
                    headers[lua_tostring(L, -2)] = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
        }

        headers["User-Agent"] = "SkidBase/1.0";
        Response res = perform_request(url, "GET", headers, {}, "");

        if (!res.success)
        {
            lua_pushstring(L, "");
            return 1;
        }

        lua_pushlstring(L, res.body.data(), res.body.size());
        return 1;
    }
}
