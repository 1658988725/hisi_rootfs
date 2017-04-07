
#include <stdio.h>    
#include <string.h>    
#include <curl/curl.h>    
#include <cdr_bubiao_analyze.h>    


/*

char* file:完整的文件路名
*/
int cdr_upload(char* url, char *FullFileName)    
{    
  CURL *curl;    
  CURLcode res;    
    
  struct curl_httppost *formpost=NULL;    
  struct curl_httppost *lastptr=NULL;    
  struct curl_slist *headerlist=NULL;    
  static const char buf[] = "Expect:";    
    
  curl_global_init(CURL_GLOBAL_ALL);    

  //printf("------------url:%s\n",sRealTimeAVBody.usServerIPAddr);
  /* Fill in the file upload field */    
  curl_formadd(&formpost,    
               &lastptr,    
               CURLFORM_COPYNAME, "file",//               CURLFORM_COPYNAME, "sendfile",        
               //CURLFORM_FILE, "man.jpg",    
               CURLFORM_FILE, FullFileName,  
               CURLFORM_END);    
    
  /* Fill in the filename field */    
  curl_formadd(&formpost,    
               &lastptr,    
               //CURLFORM_COPYNAME, "filename",    
               CURLFORM_COPYNAME, "file",    
               //CURLFORM_COPYCONTENTS, "man.jpg",    
               CURLFORM_COPYCONTENTS, FullFileName,    
               CURLFORM_END);    
    
  /* Fill in the submit field too, even if this is rarely needed */    
  curl_formadd(&formpost,    
               &lastptr,    
               CURLFORM_COPYNAME, "submit",    
               CURLFORM_COPYCONTENTS, "Submit",    
               CURLFORM_END);    
    
  curl = curl_easy_init();    
  /* initalize custom header list (stating that Expect: 100-continue is not  
     wanted */    
  headerlist = curl_slist_append(headerlist, buf);    
  if(curl) {    
    /* what URL that receives this POST */    
    //curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.85:8081/appserv/uploadImg?imgName=456123");    
    //curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.85:8081/appserv/uploadImg?imgName=12345671");        
    //curl_easy_setopt(curl, CURLOPT_URL, sRealTimeAVBody.usServerIPAddr);    
    curl_easy_setopt(curl, CURLOPT_URL, url);    
        
    //if ( (argc == 2) && (!strcmp(argv[1], "noexpectheader")) )    
      /* only disable 100-continue header if explicitly requested */    
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);    
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);    
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);  
    /* Perform the request, res will get the return code */    
    res = curl_easy_perform(curl);    
    /* Check for errors */    
    if(res != CURLE_OK) {   
      g_KakaAckBag.ucResult = 0x01;
      fprintf(stderr, "curl_easy_perform() failed: %s\n",    
              curl_easy_strerror(res));    
    }
    
    /* always cleanup */    
    curl_easy_cleanup(curl);    
    
    /* then cleanup the formpost chain */    
    curl_formfree(formpost);    
    /* free slist */    
    curl_slist_free_all (headerlist);    
  }    
  return 0;    
}   

