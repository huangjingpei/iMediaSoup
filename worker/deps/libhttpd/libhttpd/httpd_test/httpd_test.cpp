#include "http_server.h"
#include "http_connection.h"
#include <utils/KeyedVector.h>
#include <utils/String8.h>
using namespace android;

#define PORT 8080  // the port users will be connecting to


class CHttpMsgObserver : public IHttpMsgObserver {
public:
    void createErrorResp(cJSON **result_root,const char *pszError){
        *result_root = cJSON_CreateObject();
        cJSON *error_root = cJSON_CreateObject();
        cJSON_AddNumberToObject(error_root, "code", -1);
        cJSON_AddStringToObject(error_root, "message",pszError);
        cJSON_AddItemToObject(*result_root, "error", error_root);
        cJSON_AddNullToObject(*result_root, "id");
    }
    void createNullResp(cJSON **result_root){
        *result_root = cJSON_CreateObject();
        cJSON *error_root = cJSON_CreateObject();
        cJSON_AddNumberToObject(error_root, "code", 0);
        cJSON_AddNullToObject(error_root, "message");     
        cJSON_AddItemToObject(*result_root, "error", error_root);
        cJSON_AddNullToObject(*result_root, "id");
    }
    void createResp(cJSON **result_root,int _id){
        *result_root = cJSON_CreateObject();
        cJSON *error_root = cJSON_CreateObject();
        cJSON_AddNumberToObject(error_root, "code", 0);
        cJSON_AddNullToObject(error_root, "message");     
        cJSON_AddItemToObject(*result_root, "error", error_root);
        cJSON_AddNumberToObject(*result_root, "id",_id);
    }
    void sendResponse(sp<CHttpConnection> _conn, cJSON *result_root){
        char * str_result = cJSON_Print(result_root);
        _conn->Reply(str_result);
        free(str_result);
    }
    void addToParamter(const char *key, const char *value){
        if (m_paramters.indexOfKey(String8(key)) < 0) {
            m_paramters.add(String8(key), String8(value));
        } else {
            m_paramters.replaceValueFor(String8(key), String8(value));
        }
    }
    int parseJSONMsg(cJSON *item){
        int ret = 0;
        cJSON  *item_sub = NULL; 
        if ((item_sub = cJSON_GetObjectItem(item,"avs_setparam"))) {
            cJSON  * item_value = NULL;
            if ((item_value = cJSON_GetObjectItem(item_sub, "conf_id"))) {
                printf("%s %s\n", item_value->string,item_value->valuestring); 
                addToParamter(item_value->string,item_value->valuestring);
            }
            if ((item_value = cJSON_GetObjectItem(item_sub, "chan_id"))) {
                printf("%s %s\n", item_value->string,item_value->valuestring); 
                addToParamter(item_value->string,item_value->valuestring);
            }
            if ((item_value = cJSON_GetObjectItem(item_sub, "MuteVoip"))) {
                printf("%s %s\n", item_value->string,item_value->valuestring); 
                addToParamter(item_value->string,item_value->valuestring);
            }        
            if ((item_value = cJSON_GetObjectItem(item_sub, "BlockHP"))) {
                printf("%s %s\n", item_value->string,item_value->valuestring); 
                addToParamter(item_value->string,item_value->valuestring);
            }     
            if ((item_value = cJSON_GetObjectItem(item_sub, "audio_enc_param"))) {
                cJSON  * item_value_sub = NULL;
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "CN"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "MainCoder"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "Ptime"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "PayloadType"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }   
            }     

            if ((item_value = cJSON_GetObjectItem(item_sub, "audio_dec_param"))) {
                cJSON  * item_value_sub = NULL;
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "Codecs"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "PayloadType"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }   
            } 
            if ((item_value = cJSON_GetObjectItem(item_sub, "audio_transport"))) {
                cJSON  * item_value_sub = NULL;
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "BindPort"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "TargetAddr"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }   
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "SymRTP"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "RtcpMux"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "Qos"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "SrtpMode"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                }  
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "SrtpSendKey"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                } 
                if ((item_value_sub = cJSON_GetObjectItem(item_value, "SrtpRecvKey"))) {
                    printf("%s %s\n", item_value_sub->string,item_value_sub->valuestring); 
                    addToParamter(item_value_sub->string,item_value_sub->valuestring);
                } 
            }
        }else if((item_sub = cJSON_GetObjectItem(item,"runctrl"))){
            cJSON  * item_value = NULL;
            if ((item_value = cJSON_GetObjectItem(item_sub, "conf_id"))) {
                printf("%s %s\n", item_value->string,item_value->valuestring); 
                addToParamter(item_value->string,item_value->valuestring);
            }
            if ((item_value = cJSON_GetObjectItem(item_sub, "chan_id"))) {
                printf("%s %s\n", item_value->string,item_value->valuestring); 
                addToParamter(item_value->string,item_value->valuestring);
            }
            if ((item_value = cJSON_GetObjectItem(item_sub, "opt"))) {
                printf("%s %s\n", item_value->string,item_value->valuestring); 
                addToParamter(item_value->string,item_value->valuestring);
            }
        }else{
            ret = -1;
        }

        return ret;
    }

 public:
  virtual void OnReceiveHttpMsg(sp<CHttpConnection> _conn, const char *pData, size_t len) {
    const char *pContentType = _conn->getContentType();
    cJSON *result_root = NULL;
    if (strstr(pContentType,"application/json")) {
        cJSON *root; 
        char *end_ptr = NULL;
        int ret;
        _conn->dumpHttpHeadInfo();

        if ((root = cJSON_Parse_Stream(pData, &end_ptr)) != NULL) {
            if (root->type == cJSON_Object) {
               ret = parseJSONMsg(root);
               cJSON  *item_sub = NULL; 
               if ((item_sub = cJSON_GetObjectItem(root,"id"))) {//need to response
                   if (!ret) {//valid request
                       createResp(&result_root, item_sub->valueint); 
                   }else{
                       createErrorResp(&result_root,"Invalid Request" );
                   }
               }

            }else if(root->type == cJSON_Array){
                 //printf("cJSON_Array \n");
                 for (int i=0; i<cJSON_GetArraySize(root);i++) {

                     cJSON * item = cJSON_GetArrayItem(root,i);
                     if (item->type == cJSON_Object) {
                         ret = parseJSONMsg(item);
                         cJSON  *item_sub = NULL; 
                         
                        if ((item_sub = cJSON_GetObjectItem(item,"id"))) {
                            printf("----------------- i=%d request-------------------\n",i);
                            if (result_root == NULL) {
                                result_root = cJSON_CreateArray();
                            }

                            cJSON *result_root_sub;
                            if (!ret) {//valid request
                                createResp(&result_root_sub,item_sub->valueint);
                            }else{
                                createErrorResp(&result_root_sub,"Invalid Request" );
                            }
                            cJSON_AddItemToArray(result_root, result_root_sub);
                        }else{
                            printf("----------------- i=%d notice-------------------\n",i);
                        }
                     }else{
                         if (!result_root) {
                             result_root = cJSON_CreateArray();
                         }
                         cJSON *result_root_sub;
                         createErrorResp(&result_root_sub,"Invalid Request");
                         cJSON_AddItemToArray(result_root, result_root_sub);
                     }
                 }

                 if (!cJSON_GetArraySize(root)) { // the request is []
                     createErrorResp(&result_root,"Invalid Request" );
                 }
             } else { 
                 printf("root->type:%d \n", root->type );
                 createErrorResp(&result_root,"Invalid Request" );
            }
            cJSON_Delete(root); 

            if (result_root) {
                sendResponse(_conn, result_root); 
                cJSON_Delete(result_root);
            }

        }else{
            printf("cJSON_Parse_Stream failed\n");

            createErrorResp(&result_root,"Parse error" );
            sendResponse(_conn,result_root);
            cJSON_Delete(result_root);
        }

    }else if(strstr(pContentType,"image/png")){
          FILE *file = fopen("./httpbodyout.png","w");
          fwrite(pData,1,len,file);
          fclose(file);   
    }


    String8 s;
    for (int i=0; i<m_paramters.size(); ++i)
    {
        s = m_paramters.keyAt(i);
        printf("%s : %s\n", m_paramters.keyAt(i).string(), m_paramters.valueAt(i).string());
    }

  }
  void OnReleaseResouce() {};
  virtual ~CHttpMsgObserver() {}

private:
    KeyedVector<String8, String8> m_paramters;
};


int main(int argc, char* argv[])
{
    sp<CHttpMsgObserver> httpObserver = new CHttpMsgObserver();
    CHttpJsonServer::Instance()->http_server_create(httpObserver,PORT);
    CHttpJsonServer::Instance()->http_server_run();
    CHttpJsonServer::Instance()->http_server_stop();
    return 0;
}
