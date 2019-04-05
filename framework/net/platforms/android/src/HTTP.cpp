#include <bdn/Application.h>
#include <bdn/HTTP.h>
#include <bdn/HTTPRequest.h>
#include <bdn/HTTPResponse.h>
#include <bdn/config.h>
#include <bdn/entry.h>
#include <bdn/java/wrapper/NativeStrongPointer.h>
#include <ostream>

#include "../../../include/bdn/HTTPRequest.h"
#include "VolleyAdapter.h"

namespace bdn
{
    namespace net
    {
        namespace http
        {
            std::shared_ptr<HTTPResponse> request(HTTPRequest request)
            {

                static std::shared_ptr<android::JVolleyAdapter> staticVolleyAdapter =
                    std::make_shared<android::JVolleyAdapter>();

                auto response = std::make_shared<HTTPResponse>();
                response->originalRequest = request;

                staticVolleyAdapter->request(android::JVolleyAdapter::toVolleyRequestMethod(request.method),
                                             request.url, response);

                return response;
            }
        }
    }
}

extern "C" JNIEXPORT void JNICALL Java_io_boden_java_VolleyAdapter_handleResponse(JNIEnv *env, jclass rawClass,
                                                                                  jobject rawNativeResponse,
                                                                                  jint statusCode, jobject data,
                                                                                  jobject headers)
{
    bdn::platformEntryWrapper(
        [&]() {
            bdn::java::wrapper::NativeStrongPointer nativeResponse(
                bdn::java::Reference::convertExternalLocal(rawNativeResponse));

            if (std::shared_ptr<bdn::net::HTTPResponse> cResponse =
                    std::dynamic_pointer_cast<bdn::net::HTTPResponse>(nativeResponse.getBdnBasePointer())) {
                jboolean isCopy;
                cResponse->responseCode = statusCode;
                if (data != nullptr) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                    cResponse->data = env->GetStringUTFChars((jstring)data, &isCopy);
                }
                if (headers != nullptr) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                    cResponse->header = env->GetStringUTFChars((jstring)headers, &isCopy);
                }
                cResponse->originalRequest.doneHandler(cResponse);
            }
        },
        true, env);
}
