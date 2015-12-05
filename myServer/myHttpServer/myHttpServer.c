#include <sys/wait.h>
#include <signal.h>
#include "mongoose.h"
#include "fifo.h"

#define FIFO_FILENAME "tmhu"

static const char *s_http_port = "8800";
static struct mg_serve_http_opts s_http_server_opts;
static const struct mg_str s_get_method = MG_STR("GET");
//static const struct mg_str s_put_method = MG_STR("PUT");
//static const struct mg_str s_delele_method = MG_STR("DELETE");
static const struct mg_str s_post_method = MG_STR("POST");

//static void *s_db_handle = NULL;
//static const char *s_db_path = "api_server.db";


static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
  return (uri->len >= prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0)?1:0;
}

static int is_equal(const struct mg_str *s1, const struct mg_str *s2) {
  return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}

static char* null_include(const struct mg_str *s) {

  if(s->len==0) 
    return strdup("");

  char *buf = (char*)malloc(s->len+1);
  memcpy(buf,s->p,s->len);
  buf[s->len]=0;
  return buf;
}

struct TMHU {
	double temperature;
	double humidity;
};

void getTempHum(struct TMHU *pValue) {

	memset(pValue,0,sizeof(struct TMHU));
	
	// format: {DOUBLE} {DOUBLE}  ; (temperature,humidity)
	char buf[100];
	int len,i;
	int pos=0;
	
	getFifoValue(buf,sizeof(buf));
	
	len = strlen(buf);
	for(i=0;i<len;i++) {
		if(buf[i]==0x20) {
			if(pos==0) {
				buf[i]=0;
				pValue->temperature = atof(buf);
				pos=i;
			}
		}else if(i==len-1) {
			pValue->humidity = atof(&buf[pos+1]);
		}
	}
	
}

char* getRspbody(char *req_body) {
	// get temperature and humidity from FIFO
	struct TMHU value;
	getTempHum(&value);

	char* rsp_body = (char*)malloc( strlen(req_body)+255);
	sprintf(rsp_body,"{succss : request %s,temperature:%0.3f, humidity:%0.3f}",
		req_body,value.temperature,value.humidity);
	return rsp_body;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

	static const struct mg_str api_prefix = MG_STR("/api/v1");
	static const struct mg_str test_prefix = MG_STR("/tmhu");
	struct http_message *hm = (struct http_message *) ev_data;
	//struct mg_str key;

	switch (ev) {
		case MG_EV_HTTP_REQUEST:
      
		if (has_prefix(&hm->uri, &api_prefix)) {

			//key.p = hm->uri.p + api_prefix.len;
			//key.len = hm->uri.len - api_prefix.len;

			if (is_equal(&hm->method, &s_post_method)) {

				//db_op(nc, hm, &key, s_db_handle, API_OP_POST);

				if(hm->body.len>0) {
					char* req_body,*rsp_body,*out;
									   
					req_body = null_include(&hm->body);
					printf("received body from client: %s , thread_id(%u)\n",req_body,(int)pthread_self());
					
					rsp_body= getRspbody(req_body);
					
					out = (char*)malloc( strlen(rsp_body)+300);
					sprintf(out,"HTTP/1.0 200 OK\r\nContent-Length:%d\r\nContent-Type: application/json\r\n\r\n%s",
					strlen(rsp_body),rsp_body);

					free(req_body);
					free(rsp_body);

					//printf("%s\n",out);

					mg_printf(nc, "%s",out);
					free(out);

				
				}else {
					mg_printf(nc, "%s","HTTP/1.0 500\r\nContent-Length: 0\r\n\r\n");
				}  

			} else {
				mg_printf(nc, "%s","HTTP/1.0 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
			}
			
		}else if(has_prefix(&hm->uri, &test_prefix)) {

			if (is_equal(&hm->method, &s_get_method)) {

				struct TMHU value;
				getTempHum(&value);

				char rsp_body[100];
				sprintf(rsp_body,"temperature:%0.3f, humidity:%0.3f",value.temperature,value.humidity);

				char *out = (char*)malloc( strlen(rsp_body)+200);
				sprintf(out,"HTTP/1.0 200 OK\r\nContent-Length:%d\r\nContent-Type: text/html\r\n\r\n%s",
					strlen(rsp_body),rsp_body);

				mg_printf(nc, "%s",out);
				free(out);

			}else {
				mg_printf(nc, "%s","HTTP/1.0 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
			}


		}else { // has_prefix
			mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
		}
		break;
		default:
		break;
	}
}
void sig_handler(int signal)
{
	printf("cought signal %d\n",signal);

	if (signal==SIGCHLD) {
		pid_t pid;
		while ((pid = waitpid(-1, NULL, WNOHANG))>0) {
			if (errno == ECHILD) {
				break;
			}else {
				printf("child %d terminated\n", pid);
			}
		}
	}
}


int main(int argc, char *argv[]) {
	struct mg_mgr mgr;
	struct mg_connection *nc;
	int i;

	/* Open listening socket */
	mg_mgr_init(&mgr, NULL);
	nc = mg_bind(&mgr, s_http_port, ev_handler);
	mg_set_protocol_http_websocket(nc);

	/* For each new connection, execute ev_handler in a separate thread */
	mg_enable_multithreading(nc);

	s_http_server_opts.document_root = "web_root";

	/* Parse command line arguments */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-D") == 0) {
			mgr.hexdump_file = argv[++i];
		}
		/*else if (strcmp(argv[i], "-f") == 0) {
			s_db_path = argv[++i];
		}*/
		else if (strcmp(argv[i], "-r") == 0) {
			s_http_server_opts.document_root = argv[++i];
		}
	}

	/*	
	// Open database
	if ((s_db_handle = db_open(s_db_path)) == NULL) {
		fprintf(stderr, "Cannot open DB [%s]\n", s_db_path);
		exit(EXIT_FAILURE);
	}*/

	setvbuf (stdout,NULL,_IONBF,0); // for printf immediately flush
	
	signal (SIGCHLD,sig_handler);
 
	if(createFifo(FIFO_FILENAME)!=0) {
		fprintf(stderr,"createFifo fail");
		exit(1);
	}
	if(start_readFifo()!=0) {
		fprintf(stderr,"start_readFifo fail");
		exit(1);
	}

	/* Run event loop until signal is received */
	printf("Starting myHttpServer on port %s\n", s_http_port);
	for (;;)  {
		mg_mgr_poll(&mgr, 1000);
	}

	/* Cleanup */
	mg_mgr_free(&mgr);
	//db_close(&s_db_handle);

  return 0;
}

