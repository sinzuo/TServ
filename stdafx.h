using namespace std;

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#define SAFEDELETE(p) {if(p){delete(p);(p)=NULL;}}
#define SAFEDELETEARRAY(p) {if(p){delete[](p);(p)=NULL;}}
#define SAFECLOSEHANDLE(p) {if((p)!=NULL) {CloseHandle((p)); (p)=NULL;}}
#define SafeDeleteVector(p) {for(int i = 0; i < (p).size(); i ++){if((p)[i]){delete((p)[i]);}}(p).clear();}
#define LDWORD __int64
#define MOUTPUTA(format,...) {char moutbuf[2048]={0};snprintf(moutbuf,2046,format,## __VA_ARGS__);moutbuf[strlen(moutbuf)]='\n';printf(moutbuf);syslog(LOG_USER|LOG_INFO,moutbuf);}
#define MOUTPUT(format,...) {char moutbuf[2048]={0};snprintf(moutbuf,2046,format,## __VA_ARGS__);moutbuf[strlen(moutbuf)]='\n';printf(moutbuf);}
#define GetTimeStr(__ctime,__buf) sprintf((__buf),"%04d-%02d-%02d %02d:%02d:%02d",(__ctime).GetYear(),(__ctime).GetMonth(),(__ctime).GetDay(),(__ctime).GetHour(),(__ctime).GetMinute(),(__ctime).GetSecond())
#define TRIM2STR(ostr,dest) {strcpy((dest),(ostr));char* endptr=&dest[strlen(dest)-1];while((*endptr)==' '&&endptr>dest){(*(endptr--))=0;}}


