#define MAX_URL_LEN 256
#define MAX_REST_HEADER_LEN 256

#define CURL_ERROR_SIZE 256
#define CURL_GLOBAL_SSL (1<<0) /* no purpose since 7.57.0 */
#define CURL_GLOBAL_WIN32 (1<<1)

typedef enum
{
    CURLE_OK                          = 0,
    CURLE_UNSUPPORTED_PROTOCOL        = 1,
    CURLE_FAILED_INIT                 = 2,
    CURLE_URL_MALFORMAT               = 3,
    CURLE_NOT_BUILT_IN                = 4,
    CURLE_COULDNT_RESOLVE_PROXY       = 5,
    CURLE_COULDNT_RESOLVE_HOST        = 6,
    CURLE_COULDNT_CONNECT             = 7,
    CURLE_WEIRD_SERVER_REPLY          = 8,
    CURLE_REMOTE_ACCESS_DENIED        = 9,
    CURLE_FTP_ACCEPT_FAILED           = 10,
    CURLE_FTP_WEIRD_PASS_REPLY        = 11,
    CURLE_FTP_ACCEPT_TIMEOUT          = 12,
    CURLE_FTP_WEIRD_PASV_REPLY        = 13,
    CURLE_FTP_WEIRD_227_FORMAT        = 14,
    CURLE_FTP_CANT_GET_HOST           = 15,
    CURLE_HTTP2                       = 16,
    CURLE_FTP_COULDNT_SET_TYPE        = 17,
    CURLE_PARTIAL_FILE                = 18,
    CURLE_FTP_COULDNT_RETR_FILE       = 19,
    CURLE_OBSOLETE20                  = 20,
    CURLE_QUOTE_ERROR                 = 21,
    CURLE_HTTP_RETURNED_ERROR         = 22,
    CURLE_WRITE_ERROR                 = 23,
    CURLE_OBSOLETE24                  = 24,
    CURLE_UPLOAD_FAILED               = 25,
    CURLE_READ_ERROR                  = 26,
    CURLE_OUT_OF_MEMORY               = 27,
    CURLE_OPERATION_TIMEDOUT          = 28,
    CURLE_OBSOLETE29                  = 29,
    CURLE_FTP_PORT_FAILED             = 30,
    CURLE_FTP_COULDNT_USE_REST        = 31,
    CURLE_OBSOLETE32                  = 32,
    CURLE_RANGE_ERROR                 = 33,
    CURLE_HTTP_POST_ERROR             = 34,
    CURLE_SSL_CONNECT_ERROR           = 35,
    CURLE_BAD_DOWNLOAD_RESUME         = 36,
    CURLE_FILE_COULDNT_READ_FILE      = 37,
    CURLE_LDAP_CANNOT_BIND            = 38,
    CURLE_LDAP_SEARCH_FAILED          = 39,
    CURLE_OBSOLETE40                  = 40,
    CURLE_FUNCTION_NOT_FOUND          = 41,
    CURLE_ABORTED_BY_CALLBACK         = 42,
    CURLE_BAD_FUNCTION_ARGUMENT       = 43,
    CURLE_OBSOLETE44                  = 44,
    CURLE_INTERFACE_FAILED            = 45,
    CURLE_OBSOLETE46                  = 46,
    CURLE_TOO_MANY_REDIRECTS          = 47,
    CURLE_UNKNOWN_OPTION              = 48,
    CURLE_SETOPT_OPTION_SYNTAX        = 49,
    CURLE_OBSOLETE50                  = 50,
    CURLE_OBSOLETE51                  = 51,
    CURLE_GOT_NOTHING                 = 52,
    CURLE_SSL_ENGINE_NOTFOUND         = 53,
    CURLE_SSL_ENGINE_SETFAILED        = 54,
    CURLE_SEND_ERROR                  = 55,
    CURLE_RECV_ERROR                  = 56,
    CURLE_OBSOLETE57                  = 57,
    CURLE_SSL_CERTPROBLEM             = 58,
    CURLE_SSL_CIPHER                  = 59,
    CURLE_PEER_FAILED_VERIFICATION    = 60,
    CURLE_BAD_CONTENT_ENCODING        = 61,
    CURLE_OBSOLETE62                  = 62,
    CURLE_FILESIZE_EXCEEDED           = 63,
    CURLE_USE_SSL_FAILED              = 64,
    CURLE_SEND_FAIL_REWIND            = 65,
    CURLE_SSL_ENGINE_INITFAILED       = 66,
    CURLE_LOGIN_DENIED                = 67,
    CURLE_TFTP_NOTFOUND               = 68,
    CURLE_TFTP_PERM                   = 69,
    CURLE_REMOTE_DISK_FULL            = 70,
    CURLE_TFTP_ILLEGAL                = 71,
    CURLE_TFTP_UNKNOWNID              = 72,
    CURLE_REMOTE_FILE_EXISTS          = 73,
    CURLE_TFTP_NOSUCHUSER             = 74,
    CURLE_OBSOLETE75                  = 75,
    CURLE_OBSOLETE76                  = 76,
    CURLE_SSL_CACERT_BADFILE          = 77,
    CURLE_REMOTE_FILE_NOT_FOUND       = 78,
    CURLE_SSH                         = 79,
    CURLE_SSL_SHUTDOWN_FAILED         = 80,
    CURLE_AGAIN                       = 81,
    CURLE_SSL_CRL_BADFILE             = 82,
    CURLE_SSL_ISSUER_ERROR            = 83,
    CURLE_FTP_PRET_FAILED             = 84,
    CURLE_RTSP_CSEQ_ERROR             = 85,
    CURLE_RTSP_SESSION_ERROR          = 86,
    CURLE_FTP_BAD_FILE_LIST           = 87,
    CURLE_CHUNK_FAILED                = 88,
    CURLE_NO_CONNECTION_AVAILABLE     = 89,
    CURLE_SSL_PINNEDPUBKEYNOTMATCH    = 90,
    CURLE_SSL_INVALIDCERTSTATUS       = 91,
    CURLE_HTTP2_STREAM                = 92,
    CURLE_RECURSIVE_API_CALL          = 93,
    CURLE_AUTH_ERROR                  = 94,
    CURLE_HTTP3                       = 95,
    CURLE_QUIC_CONNECT_ERROR          = 96,
    CURLE_PROXY                       = 97,
    CURLE_SSL_CLIENTCERT              = 98,
    CURLE_UNRECOVERABLE_POLL          = 99,
    CURL_LAST                         = 100,
} CURLcode;

typedef enum
{
    CURLM_CALL_MULTI_PERFORM    = -1,
    CURLM_OK                    = 0,
    CURLM_BAD_HANDLE            = 1,
    CURLM_BAD_EASY_HANDLE       = 2,
    CURLM_OUT_OF_MEMORY         = 3,
    CURLM_INTERNAL_ERROR        = 4,
    CURLM_BAD_SOCKET            = 5,
    CURLM_UNKNOWN_OPTION        = 6,
    CURLM_ADDED_ALREADY         = 7,
    CURLM_RECURSIVE_API_CALL    = 8,
    CURLM_WAKEUP_FAILURE        = 9,
    CURLM_BAD_FUNCTION_ARGUMENT = 10,
    CURLM_ABORTED_BY_CALLBACK   = 11,
    CURLM_UNRECOVERABLE_POLL    = 12,
    CURLM_LAST                  = 13,
} CURLMcode;

typedef enum
{
    CURLOPT_WRITEDATA                   = 1 + 10000,
    CURLOPT_URL                         = 2 + 10000,
    CURLOPT_PORT                        = 3 + 0,
    CURLOPT_PROXY                       = 4 + 10000,
    CURLOPT_USERPWD                     = 5 + 10000,
    CURLOPT_PROXYUSERPWD                = 6 + 10000,
    CURLOPT_RANGE                       = 7 + 10000,
    CURLOPT_READDATA                    = 9 + 10000,
    CURLOPT_ERRORBUFFER                 = 10 + 10000,
    CURLOPT_WRITEFUNCTION               = 11 + 20000,
    CURLOPT_READFUNCTION                = 12 + 20000,
    CURLOPT_TIMEOUT                     = 13 + 0,
    CURLOPT_INFILESIZE                  = 14 + 0,
    CURLOPT_POSTFIELDS                  = 15 + 10000,
    CURLOPT_REFERER                     = 16 + 10000,
    CURLOPT_FTPPORT                     = 17 + 10000,
    CURLOPT_USERAGENT                   = 18 + 10000,
    CURLOPT_LOW_SPEED_LIMIT             = 19 + 0,
    CURLOPT_LOW_SPEED_TIME              = 20 + 0,
    CURLOPT_RESUME_FROM                 = 21 + 0,
    CURLOPT_COOKIE                      = 22 + 10000,
    CURLOPT_HTTPHEADER                  = 23 + 10000,
    CURLOPT_HTTPPOST                    = 24 + 10000,
    CURLOPT_SSLCERT                     = 25 + 10000,
    CURLOPT_KEYPASSWD                   = 26 + 10000,
    CURLOPT_CRLF                        = 27 + 0,
    CURLOPT_QUOTE                       = 28 + 10000,
    CURLOPT_HEADERDATA                  = 29 + 10000,
    CURLOPT_COOKIEFILE                  = 31 + 10000,
    CURLOPT_SSLVERSION                  = 32 + 0,
    CURLOPT_TIMECONDITION               = 33 + 0,
    CURLOPT_TIMEVALUE                   = 34 + 0,
    CURLOPT_CUSTOMREQUEST               = 36 + 10000,
    CURLOPT_STDERR                      = 37 + 10000,
    CURLOPT_POSTQUOTE                   = 39 + 10000,
    CURLOPT_OBSOLETE40                  = 40 + 10000,
    CURLOPT_VERBOSE                     = 41 + 0,
    CURLOPT_HEADER                      = 42 + 0,
    CURLOPT_NOPROGRESS                  = 43 + 0,
    CURLOPT_NOBODY                      = 44 + 0,
    CURLOPT_FAILONERROR                 = 45 + 0,
    CURLOPT_UPLOAD                      = 46 + 0,
    CURLOPT_POST                        = 47 + 0,
    CURLOPT_DIRLISTONLY                 = 48 + 0,
    CURLOPT_APPEND                      = 50 + 0,
    CURLOPT_NETRC                       = 51 + 0,
    CURLOPT_FOLLOWLOCATION              = 52 + 0,
    CURLOPT_TRANSFERTEXT                = 53 + 0,
    CURLOPT_PUT                         = 54 + 0,
    CURLOPT_PROGRESSFUNCTION            = 56 + 20000,
    CURLOPT_XFERINFODATA                = 57 + 10000,
    CURLOPT_AUTOREFERER                 = 58 + 0,
    CURLOPT_PROXYPORT                   = 59 + 0,
    CURLOPT_POSTFIELDSIZE               = 60 + 0,
    CURLOPT_HTTPPROXYTUNNEL             = 61 + 0,
    CURLOPT_INTERFACE                   = 62 + 10000,
    CURLOPT_KRBLEVEL                    = 63 + 10000,
    CURLOPT_SSL_VERIFYPEER              = 64 + 0,
    CURLOPT_CAINFO                      = 65 + 10000,
    CURLOPT_MAXREDIRS                   = 68 + 0,
    CURLOPT_FILETIME                    = 69 + 0,
    CURLOPT_TELNETOPTIONS               = 70 + 10000,
    CURLOPT_MAXCONNECTS                 = 71 + 0,
    CURLOPT_OBSOLETE72                  = 72 + 0,
    CURLOPT_FRESH_CONNECT               = 74 + 0,
    CURLOPT_FORBID_REUSE                = 75 + 0,
    CURLOPT_RANDOM_FILE                 = 76 + 10000,
    CURLOPT_EGDSOCKET                   = 77 + 10000,
    CURLOPT_CONNECTTIMEOUT              = 78 + 0,
    CURLOPT_HEADERFUNCTION              = 79 + 20000,
    CURLOPT_HTTPGET                     = 80 + 0,
    CURLOPT_SSL_VERIFYHOST              = 81 + 0,
    CURLOPT_COOKIEJAR                   = 82 + 10000,
    CURLOPT_SSL_CIPHER_LIST             = 83 + 10000,
    CURLOPT_HTTP_VERSION                = 84 + 0,
    CURLOPT_FTP_USE_EPSV                = 85 + 0,
    CURLOPT_SSLCERTTYPE                 = 86 + 10000,
    CURLOPT_SSLKEY                      = 87 + 10000,
    CURLOPT_SSLKEYTYPE                  = 88 + 10000,
    CURLOPT_SSLENGINE                   = 89 + 10000,
    CURLOPT_SSLENGINE_DEFAULT           = 90 + 0,
    CURLOPT_DNS_USE_GLOBAL_CACHE        = 91 + 0,
    CURLOPT_DNS_CACHE_TIMEOUT           = 92 + 0,
    CURLOPT_PREQUOTE                    = 93 + 10000,
    CURLOPT_DEBUGFUNCTION               = 94 + 20000,
    CURLOPT_DEBUGDATA                   = 95 + 10000,
    CURLOPT_COOKIESESSION               = 96 + 0,
    CURLOPT_CAPATH                      = 97 + 10000,
    CURLOPT_BUFFERSIZE                  = 98 + 0,
    CURLOPT_NOSIGNAL                    = 99 + 0,
    CURLOPT_SHARE                       = 100 + 10000,
    CURLOPT_PROXYTYPE                   = 101 + 0,
    CURLOPT_ACCEPT_ENCODING             = 102 + 10000,
    CURLOPT_PRIVATE                     = 103 + 10000,
    CURLOPT_HTTP200ALIASES              = 104 + 10000,
    CURLOPT_UNRESTRICTED_AUTH           = 105 + 0,
    CURLOPT_FTP_USE_EPRT                = 106 + 0,
    CURLOPT_HTTPAUTH                    = 107 + 0,
    CURLOPT_SSL_CTX_FUNCTION            = 108 + 20000,
    CURLOPT_SSL_CTX_DATA                = 109 + 10000,
    CURLOPT_FTP_CREATE_MISSING_DIRS     = 110 + 0,
    CURLOPT_PROXYAUTH                   = 111 + 0,
    CURLOPT_SERVER_RESPONSE_TIMEOUT     = 112 + 0,
    CURLOPT_IPRESOLVE                   = 113 + 0,
    CURLOPT_MAXFILESIZE                 = 114 + 0,
    CURLOPT_INFILESIZE_LARGE            = 115 + 30000,
    CURLOPT_RESUME_FROM_LARGE           = 116 + 30000,
    CURLOPT_MAXFILESIZE_LARGE           = 117 + 30000,
    CURLOPT_NETRC_FILE                  = 118 + 10000,
    CURLOPT_USE_SSL                     = 119 + 0,
    CURLOPT_POSTFIELDSIZE_LARGE         = 120 + 30000,
    CURLOPT_TCP_NODELAY                 = 121 + 0,
    CURLOPT_FTPSSLAUTH                  = 129 + 0,
    CURLOPT_IOCTLFUNCTION               = 130 + 20000,
    CURLOPT_IOCTLDATA                   = 131 + 10000,
    CURLOPT_FTP_ACCOUNT                 = 134 + 10000,
    CURLOPT_COOKIELIST                  = 135 + 10000,
    CURLOPT_IGNORE_CONTENT_LENGTH       = 136 + 0,
    CURLOPT_FTP_SKIP_PASV_IP            = 137 + 0,
    CURLOPT_FTP_FILEMETHOD              = 138 + 0,
    CURLOPT_LOCALPORT                   = 139 + 0,
    CURLOPT_LOCALPORTRANGE              = 140 + 0,
    CURLOPT_CONNECT_ONLY                = 141 + 0,
    CURLOPT_CONV_FROM_NETWORK_FUNCTION  = 142 + 20000,
    CURLOPT_CONV_TO_NETWORK_FUNCTION    = 143 + 20000,
    CURLOPT_CONV_FROM_UTF8_FUNCTION     = 144 + 20000,
    CURLOPT_MAX_SEND_SPEED_LARGE        = 145 + 30000,
    CURLOPT_MAX_RECV_SPEED_LARGE        = 146 + 30000,
    CURLOPT_FTP_ALTERNATIVE_TO_USER     = 147 + 10000,
    CURLOPT_SOCKOPTFUNCTION             = 148 + 20000,
    CURLOPT_SOCKOPTDATA                 = 149 + 10000,
    CURLOPT_SSL_SESSIONID_CACHE         = 150 + 0,
    CURLOPT_SSH_AUTH_TYPES              = 151 + 0,
    CURLOPT_SSH_PUBLIC_KEYFILE          = 152 + 10000,
    CURLOPT_SSH_PRIVATE_KEYFILE         = 153 + 10000,
    CURLOPT_FTP_SSL_CCC                 = 154 + 0,
    CURLOPT_TIMEOUT_MS                  = 155 + 0,
    CURLOPT_CONNECTTIMEOUT_MS           = 156 + 0,
    CURLOPT_HTTP_TRANSFER_DECODING      = 157 + 0,
    CURLOPT_HTTP_CONTENT_DECODING       = 158 + 0,
    CURLOPT_NEW_FILE_PERMS              = 159 + 0,
    CURLOPT_NEW_DIRECTORY_PERMS         = 160 + 0,
    CURLOPT_POSTREDIR                   = 161 + 0,
    CURLOPT_SSH_HOST_PUBLIC_KEY_MD5     = 162 + 10000,
    CURLOPT_OPENSOCKETFUNCTION          = 163 + 20000,
    CURLOPT_OPENSOCKETDATA              = 164 + 10000,
    CURLOPT_COPYPOSTFIELDS              = 165 + 10000,
    CURLOPT_PROXY_TRANSFER_MODE         = 166 + 0,
    CURLOPT_SEEKFUNCTION                = 167 + 20000,
    CURLOPT_SEEKDATA                    = 168 + 10000,
    CURLOPT_CRLFILE                     = 169 + 10000,
    CURLOPT_ISSUERCERT                  = 170 + 10000,
    CURLOPT_ADDRESS_SCOPE               = 171 + 0,
    CURLOPT_CERTINFO                    = 172 + 0,
    CURLOPT_USERNAME                    = 173 + 10000,
    CURLOPT_PASSWORD                    = 174 + 10000,
    CURLOPT_PROXYUSERNAME               = 175 + 10000,
    CURLOPT_PROXYPASSWORD               = 176 + 10000,
    CURLOPT_NOPROXY                     = 177 + 10000,
    CURLOPT_TFTP_BLKSIZE                = 178 + 0,
    CURLOPT_SOCKS5_GSSAPI_SERVICE       = 179 + 10000,
    CURLOPT_SOCKS5_GSSAPI_NEC           = 180 + 0,
    CURLOPT_PROTOCOLS                   = 181 + 0,
    CURLOPT_REDIR_PROTOCOLS             = 182 + 0,
    CURLOPT_SSH_KNOWNHOSTS              = 183 + 10000,
    CURLOPT_SSH_KEYFUNCTION             = 184 + 20000,
    CURLOPT_SSH_KEYDATA                 = 185 + 10000,
    CURLOPT_MAIL_FROM                   = 186 + 10000,
    CURLOPT_MAIL_RCPT                   = 187 + 10000,
    CURLOPT_FTP_USE_PRET                = 188 + 0,
    CURLOPT_RTSP_REQUEST                = 189 + 0,
    CURLOPT_RTSP_SESSION_ID             = 190 + 10000,
    CURLOPT_RTSP_STREAM_URI             = 191 + 10000,
    CURLOPT_RTSP_TRANSPORT              = 192 + 10000,
    CURLOPT_RTSP_CLIENT_CSEQ            = 193 + 0,
    CURLOPT_RTSP_SERVER_CSEQ            = 194 + 0,
    CURLOPT_INTERLEAVEDATA              = 195 + 10000,
    CURLOPT_INTERLEAVEFUNCTION          = 196 + 20000,
    CURLOPT_WILDCARDMATCH               = 197 + 0,
    CURLOPT_CHUNK_BGN_FUNCTION          = 198 + 20000,
    CURLOPT_CHUNK_END_FUNCTION          = 199 + 20000,
    CURLOPT_FNMATCH_FUNCTION            = 200 + 20000,
    CURLOPT_CHUNK_DATA                  = 201 + 10000,
    CURLOPT_FNMATCH_DATA                = 202 + 10000,
    CURLOPT_RESOLVE                     = 203 + 10000,
    CURLOPT_TLSAUTH_USERNAME            = 204 + 10000,
    CURLOPT_TLSAUTH_PASSWORD            = 205 + 10000,
    CURLOPT_TLSAUTH_TYPE                = 206 + 10000,
    CURLOPT_TRANSFER_ENCODING           = 207 + 0,
    CURLOPT_CLOSESOCKETFUNCTION         = 208 + 20000,
    CURLOPT_CLOSESOCKETDATA             = 209 + 10000,
    CURLOPT_GSSAPI_DELEGATION           = 210 + 0,
    CURLOPT_DNS_SERVERS                 = 211 + 10000,
    CURLOPT_ACCEPTTIMEOUT_MS            = 212 + 0,
    CURLOPT_TCP_KEEPALIVE               = 213 + 0,
    CURLOPT_TCP_KEEPIDLE                = 214 + 0,
    CURLOPT_TCP_KEEPINTVL               = 215 + 0,
    CURLOPT_SSL_OPTIONS                 = 216 + 0,
    CURLOPT_MAIL_AUTH                   = 217 + 10000,
    CURLOPT_SASL_IR                     = 218 + 0,
    CURLOPT_XFERINFOFUNCTION            = 219 + 20000,
    CURLOPT_XOAUTH2_BEARER              = 220 + 10000,
    CURLOPT_DNS_INTERFACE               = 221 + 10000,
    CURLOPT_DNS_LOCAL_IP4               = 222 + 10000,
    CURLOPT_DNS_LOCAL_IP6               = 223 + 10000,
    CURLOPT_LOGIN_OPTIONS               = 224 + 10000,
    CURLOPT_SSL_ENABLE_NPN              = 225 + 0,
    CURLOPT_SSL_ENABLE_ALPN             = 226 + 0,
    CURLOPT_EXPECT_100_TIMEOUT_MS       = 227 + 0,
    CURLOPT_PROXYHEADER                 = 228 + 10000,
    CURLOPT_HEADEROPT                   = 229 + 0,
    CURLOPT_PINNEDPUBLICKEY             = 230 + 10000,
    CURLOPT_UNIX_SOCKET_PATH            = 231 + 10000,
    CURLOPT_SSL_VERIFYSTATUS            = 232 + 0,
    CURLOPT_SSL_FALSESTART              = 233 + 0,
    CURLOPT_PATH_AS_IS                  = 234 + 0,
    CURLOPT_PROXY_SERVICE_NAME          = 235 + 10000,
    CURLOPT_SERVICE_NAME                = 236 + 10000,
    CURLOPT_PIPEWAIT                    = 237 + 0,
    CURLOPT_DEFAULT_PROTOCOL            = 238 + 10000,
    CURLOPT_STREAM_WEIGHT               = 239 + 0,
    CURLOPT_STREAM_DEPENDS              = 240 + 10000,
    CURLOPT_STREAM_DEPENDS_E            = 241 + 10000,
    CURLOPT_TFTP_NO_OPTIONS             = 242 + 0,
    CURLOPT_CONNECT_TO                  = 243 + 10000,
    CURLOPT_TCP_FASTOPEN                = 244 + 0,
    CURLOPT_KEEP_SENDING_ON_ERROR       = 245 + 0,
    CURLOPT_PROXY_CAINFO                = 246 + 10000,
    CURLOPT_PROXY_CAPATH                = 247 + 10000,
    CURLOPT_PROXY_SSL_VERIFYPEER        = 248 + 0,
    CURLOPT_PROXY_SSL_VERIFYHOST        = 249 + 0,
    CURLOPT_PROXY_SSLVERSION            = 250 + 0,
    CURLOPT_PROXY_TLSAUTH_USERNAME      = 251 + 10000,
    CURLOPT_PROXY_TLSAUTH_PASSWORD      = 252 + 10000,
    CURLOPT_PROXY_TLSAUTH_TYPE          = 253 + 10000,
    CURLOPT_PROXY_SSLCERT               = 254 + 10000,
    CURLOPT_PROXY_SSLCERTTYPE           = 255 + 10000,
    CURLOPT_PROXY_SSLKEY                = 256 + 10000,
    CURLOPT_PROXY_SSLKEYTYPE            = 257 + 10000,
    CURLOPT_PROXY_KEYPASSWD             = 258 + 10000,
    CURLOPT_PROXY_SSL_CIPHER_LIST       = 259 + 10000,
    CURLOPT_PROXY_CRLFILE               = 260 + 10000,
    CURLOPT_PROXY_SSL_OPTIONS           = 261 + 0,
    CURLOPT_PRE_PROXY                   = 262 + 10000,
    CURLOPT_PROXY_PINNEDPUBLICKEY       = 263 + 10000,
    CURLOPT_ABSTRACT_UNIX_SOCKET        = 264 + 10000,
    CURLOPT_SUPPRESS_CONNECT_HEADERS    = 265 + 0,
    CURLOPT_REQUEST_TARGET              = 266 + 10000,
    CURLOPT_SOCKS5_AUTH                 = 267 + 0,
    CURLOPT_SSH_COMPRESSION             = 268 + 0,
    CURLOPT_MIMEPOST                    = 269 + 10000,
    CURLOPT_TIMEVALUE_LARGE             = 270 + 30000,
    CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS   = 271 + 0,
    CURLOPT_RESOLVER_START_FUNCTION     = 272 + 20000,
    CURLOPT_RESOLVER_START_DATA         = 273 + 10000,
    CURLOPT_HAPROXYPROTOCOL             = 274 + 0,
    CURLOPT_DNS_SHUFFLE_ADDRESSES       = 275 + 0,
    CURLOPT_TLS13_CIPHERS               = 276 + 10000,
    CURLOPT_PROXY_TLS13_CIPHERS         = 277 + 10000,
    CURLOPT_DISALLOW_USERNAME_IN_URL    = 278 + 0,
    CURLOPT_DOH_URL                     = 279 + 10000,
    CURLOPT_UPLOAD_BUFFERSIZE           = 280 + 0,
    CURLOPT_UPKEEP_INTERVAL_MS          = 281 + 0,
    CURLOPT_CURLU                       = 282 + 10000,
    CURLOPT_TRAILERFUNCTION             = 283 + 20000,
    CURLOPT_TRAILERDATA                 = 284 + 10000,
    CURLOPT_HTTP09_ALLOWED              = 285 + 0,
    CURLOPT_ALTSVC_CTRL                 = 286 + 0,
    CURLOPT_ALTSVC                      = 287 + 10000,
    CURLOPT_MAXAGE_CONN                 = 288 + 0,
    CURLOPT_SASL_AUTHZID                = 289 + 10000,
    CURLOPT_MAIL_RCPT_ALLOWFAILS        = 290 + 0,
    CURLOPT_SSLCERT_BLOB                = 291 + 40000,
    CURLOPT_SSLKEY_BLOB                 = 292 + 40000,
    CURLOPT_PROXY_SSLCERT_BLOB          = 293 + 40000,
    CURLOPT_PROXY_SSLKEY_BLOB           = 294 + 40000,
    CURLOPT_ISSUERCERT_BLOB             = 295 + 40000,
    CURLOPT_PROXY_ISSUERCERT            = 296 + 10000,
    CURLOPT_PROXY_ISSUERCERT_BLOB       = 297 + 40000,
    CURLOPT_SSL_EC_CURVES               = 298 + 10000,
    CURLOPT_HSTS_CTRL                   = 299 + 0,
    CURLOPT_HSTS                        = 300 + 10000,
    CURLOPT_HSTSREADFUNCTION            = 301 + 20000,
    CURLOPT_HSTSREADDATA                = 302 + 10000,
    CURLOPT_HSTSWRITEFUNCTION           = 303 + 20000,
    CURLOPT_HSTSWRITEDATA               = 304 + 10000,
    CURLOPT_AWS_SIGV4                   = 305 + 10000,
    CURLOPT_DOH_SSL_VERIFYPEER          = 306 + 0,
    CURLOPT_DOH_SSL_VERIFYHOST          = 307 + 0,
    CURLOPT_DOH_SSL_VERIFYSTATUS        = 308 + 0,
    CURLOPT_CAINFO_BLOB                 = 309 + 40000,
    CURLOPT_PROXY_CAINFO_BLOB           = 310 + 40000,
    CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256  = 311 + 10000,
    CURLOPT_PREREQFUNCTION              = 312 + 20000,
    CURLOPT_PREREQDATA                  = 313 + 10000,
    CURLOPT_MAXLIFETIME_CONN            = 314 + 0,
    CURLOPT_MIME_OPTIONS                = 315 + 0,
    CURLOPT_SSH_HOSTKEYFUNCTION         = 316 + 20000,
    CURLOPT_SSH_HOSTKEYDATA             = 317 + 10000,
    CURLOPT_PROTOCOLS_STR               = 318 + 10000,
    CURLOPT_REDIR_PROTOCOLS_STR         = 319 + 10000,
    CURLOPT_WS_OPTIONS                  = 320 + 0,
    CURLOPT_CA_CACHE_TIMEOUT            = 321 + 0,
    CURLOPT_QUICK_EXIT                  = 322 + 0,
    CURLOPT_HAPROXY_CLIENT_IP           = 323 + 10000,
    CURLOPT_LASTENTRY
} CURLoption;

typedef enum
{
    CURLMSG_NONE = 0,
    CURLMSG_DONE = 1,
    CURLMSG_LAST = 2,
} CURLMSG;

typedef void CURL;
typedef void CURLM;

typedef struct curl_slist
{
    char *data;
    struct curl_slist *next;
} curl_slist;

typedef struct curl_waitfd
{
    curl_socket_t fd;
    short events;
    short revents;
    int padding_;
} curl_waitfd;

typedef struct CURLMsg
{
    CURLMSG msg;
    int padding_;
    CURL *easy_handle;
    union
    {
        void *whatever;
        CURLcode result;
    } data;
} CURLMsg;

typedef curl_slist *CurlSListAppend(curl_slist *list, const char *string);
typedef CURL *CurlEasyInit(void);
typedef CURLcode CurlEasySetOpt(CURL *handle, CURLoption option, ...);
typedef CURLcode CurlGlobalInit(long flags);
typedef CURLM *CurlMultiInit(void);
typedef CURLMcode CurlMultiAddHandle(CURLM *multi_handle, CURL *easy_handle);
typedef CURLMcode CurlMultiCleanup(CURLM *multi_handle);
typedef CURLMcode CurlMultiPerform(CURLM *multi_handle, int *running_handles);
typedef CURLMcode CurlMultiPoll(CURLM *multi_handle, struct curl_waitfd *extra_fds, unsigned int extra_nfds, int timeout_ms, int *numfds);
typedef CURLMcode CurlMultiRemoveHandle(CURLM *multi_handle, CURL *easy_handle);
typedef CURLMsg *CurlMultiInfoRead(CURLM *multi_handle, int *msgs_in_queue);
typedef void CurlEasyCleanup(CURL *handle);
typedef void CurlGlobalCleanup(void);
typedef void CurlSListFreeAll(curl_slist *list);

typedef struct CurlWorker
{
    CURL *handle;
    curl_slist *header_list;
    GrowableBuffer response;
    char error[CURL_ERROR_SIZE];
} CurlWorker;

typedef struct CurlGroup
{
    int worker_count;
    int padding_;
    CURLM *handle;
    CurlWorker *workers;
} CurlGroup;

typedef struct CurlCode
{
    CurlEasyCleanup *curl_easy_cleanup;
    CurlEasyInit *curl_easy_init;
    CurlEasySetOpt *curl_easy_setopt;
    CurlGlobalCleanup *curl_global_cleanup;
    CurlGlobalInit *curl_global_init;
    CurlMultiAddHandle *curl_multi_add_handle;
    CurlMultiCleanup *curl_multi_cleanup;
    CurlMultiInfoRead *curl_multi_info_read;
    CurlMultiInit *curl_multi_init;
    CurlMultiPerform *curl_multi_perform;
    CurlMultiPoll *curl_multi_poll;
    CurlMultiRemoveHandle *curl_multi_remove_handle;
    CurlSListAppend *curl_slist_append;
    CurlSListFreeAll *curl_slist_free_all;
} CurlCode;

static CurlCode curl = {0};
static char global_certificate_path[MAX_PATH_LEN];