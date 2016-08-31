#include "qsavefile.h"
#include "sqlite3.h"
//#include "qstring.h"
//#include "qdatetime.h"
#include "sys/syscall.h"
#include "sys/stat.h"
#define SAVE_USER 254
#define DEBUG  1

#define DB_NAME "./dvo_ipcamera_save.db"

void time_to_string(time_t time1, char *szTime)
{
  struct tm tm1;

#ifdef WIN32
    tm1 = *localtime(&time1);
#else
    localtime_r(&time1, &tm1 );
#endif
    sprintf( szTime, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",
           tm1.tm_year+1900, tm1.tm_mon+1, tm1.tm_mday,
             tm1.tm_hour, tm1.tm_min,tm1.tm_sec);
}

time_t string_to_time(char * szTime)
{
    struct tm tm1;
    time_t time1;


    sscanf(szTime, "%4d-%2d-%2d %2d:%2d:%2d",
          &tm1.tm_year,
          &tm1.tm_mon,
          &tm1.tm_mday,
          &tm1.tm_hour,
          &tm1.tm_min,
          &tm1.tm_sec);

    tm1.tm_year -= 1900;
    tm1.tm_mon --;


    tm1.tm_isdst=-1;

    time1 = mktime(&tm1);
    return time1;
}


QSavefile::QSavefile()
{
    char* msg = NULL ;
    int ret = 0 ;

    ret = sqlite3_open(DB_NAME, &db);
    if (ret){
        fprintf(stderr, "error open datebase:%s\n.", DB_NAME) ;
        exit(0) ;
    }
    else{
        fprintf(stderr, "successfully open datebase.\n") ;
        if(sqlite3_exec(db,"select * from save",0,0,NULL) != 0)
        {
            sqlite3_exec(db,"create table save(no integer primary key autoincrement , time datetime,size integer,path nvarchar(512))",NULL,NULL,NULL);
        }
    }
    cmd_data_callback = new db_cmd_callback_t();
    cmd_data_callback->data = new db_cmd_info_t();
    head_cmd_list = new struct db_cmd_list_t();
    head_cmd_list->next = NULL;

}

void QSavefile::bound(QSlidingWindow *slidingwnd)
{
    sliding_window = slidingwnd;
    current_user  = 0;
    save_consume = append_user(SAVE_USER);
}
void QSavefile::adduser(int user)
{
    save_consume = append_user(user);
}
//删附数据及文件
void QSavefile::delete_table()
{
    char cmd[1024];
    int count = 0;

    cmd_data_callback->cmd = DB_CMD_CALLBACK_COUNT;
    sprintf(cmd,"select count(*) from save");
    db_exec(cmd,cmd_data_callback);
    count = cmd_data_callback->data->size;

    for(int i = 0;i < count;i++)
    {
        sprintf(cmd,"select path, min(no) from save");
        cmd_data_callback->cmd = DB_CMD_CALLBACK_DELETE;
        db_exec(cmd,cmd_data_callback);
        sprintf(cmd,"/bin/rm -f %s",cmd_data_callback->data->path);
        printf("delete:%s\n",cmd);
        system(cmd);

        sprintf(cmd,"delete from save where no in (select min(no) from save)");
        cmd_data_callback->cmd = DB_CMD_CALLBACK_NONE;
        db_exec(cmd,cmd_data_callback);
    }


}
//在定时保存情况下，设定保存文件的磁盘空间，如果超过，则进行删除
void QSavefile::set_savedisk_size(long disk_size_kb)
{
    disk_size = disk_size_kb;
}
//添加一个用作数据保存的用户
QSlidingWindowConsume *QSavefile::append_user(int user_no)
{
    if(current_user == 0)
    {
        current_user = user_no;
        int ok = sliding_window->consume_linklist_append(current_user);
        if(ok > 0)
        {
            return sliding_window->consume_linklist_getConsume(current_user);
        }
        else
        {
            current_user = 0;
            return NULL;
        }
    }
}
//启动保存线程
void QSavefile::start_save()
{
    if(save_consume != NULL)
    {
      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_create(&save_pthread_id,&attr,save_run,this);
      pthread_attr_destroy (&attr);
    }
}
//设定保存模式：
//time_s:如果>0:为定时保存，＝0只存一个文件
//保存的文件名，如果是定时保存，文件名格式:filename_yyyy-MM-dd_HH-mm-ss.h.264
//save_head_flag:保存时是否需要保存协议头；1:保存 0:不保存
void QSavefile::set_save_model(int time_s, char *filename,int save_head_flag)
{
    file_name = (char *)malloc(sizeof(char) * 1024);
    time_sec = time_s;
    sprintf(file_name,"/Users/qianzhengyang/%s",filename);
    savestatus = 1;
    head_inc = save_head_flag;
}

void QSavefile::stop_save()
{
    savestatus = 0;
}
//数据保存线程
void *QSavefile::save_run(void *ptr)
{
    QSavefile *pthis = (QSavefile *)ptr;
    char *buffer = (char *)malloc(sizeof(char) * 1024 * 1024);
    pthis->save_consume->read_init();
    struct head_buf_t  *head ;//= new struct head_buf_t;
    int start_save = 0;
    int _head;
    char filename[1024];
    if(pthis->head_inc == 1)
        _head = 0;
    else if(pthis->head_inc == 0)
        _head = HEAD_SIZE;
    printf("QSavefile pthread start! pid:%ud \n",syscall(SYS_gettid));
    //将该线程进行CPU绑定
    //taskset -p mask pid: 0x3c = 0b00111100  即绑定cpu2 cpu3 cpu4 cpu5
    char *cmd_system = (char *)malloc(sizeof(char)*1024);
    sprintf(cmd_system,"/bin/taskset -p 0x3c %ud",syscall(SYS_gettid));
    //system(cmd_system);
    free(cmd_system);
    struct stat buf;
    while(pthis->savestatus)
    {
        int len = pthis->save_consume->read_data_and_head(buffer);
        if(start_save == 0)
        {
            //memcpy(head,buffer,HEAD_SIZE);
            head = (struct head_buf_t *)buffer;
            printf("(QSave)frame type:%d \n",head->frame.frame_type);
            if(head->frame.frame_type == 1) //判定从I帧开始存数据
            {
                if(pthis->time_sec > 0)  //如果设定时间>0，表示是定时存数据，如果是==0，则表示只存一个文件
                {
                    //pthis->start_time = QDateTime::currentDateTime();//获取系统现在的时间
                    //QString str = pthis->start_time.toString("yyyy-MM-dd_HH-mm-ss"); //设置显示格式
                    //sprintf(filename,"%s_%s.h264",pthis->file_name,str.toLatin1().data());//文件名：设定文件名_日期_时间.h264
                    time_t timep;
                    time(&timep);
                    struct tm *p;
                    p = localtime(&timep);
                    char *str_time = (char *)malloc(sizeof(char) * 20);
                    time_to_string(timep,str_time);

                    pthis->save_file = fopen(filename,"w");
                    start_save = 1;
                    //将文件记录保存到数据库
                    char *cmd = (char *)malloc(sizeof(char) * 1024 *1024);
                    //QString str_time = pthis->start_time.toString("yyyy-MM-dd hh:mm:ss"); //设置显示格式
                    //sprintf(cmd,"insert into save(time,path) values('%s','%s') ",str_time.toLatin1().data(),filename);
                    sprintf(cmd,"insert into save(time,path) values('%s','%s') ",str_time,filename);
                    pthis->cmd_data_callback->cmd = DB_CMD_CALLBACK_NONE;
                    pthis->db_exec(cmd,pthis->cmd_data_callback);
                    pthis->start_time = mktime(p);
                    free(cmd);
                    free(str_time);
                }
                if(pthis->time_sec == 0) //只存一个文件
                {
                    pthis->save_file = fopen(pthis->file_name,"w");
                    start_save = 1;
                }
            }
        }
        if(len < 0)
        {
            fclose(pthis->save_file);
        }
        if(start_save == 1)
        {
            if(pthis->time_sec > 0)  //定时保存
            {
                //pthis->current_time = QDateTime::currentDateTime();
                time_t cur_time;
                time(&cur_time);
                struct tm *p;
                p = localtime(&cur_time);  //将cur_time转成现本地世界时间
                pthis->current_time = mktime(p);  //将本地时间转成从1970-1-1 00:00:00开始的秒数
                int cur_seconds;
                //cur_seconds = pthis->start_time.secsTo(pthis->current_time);
                cur_seconds = pthis->current_time - pthis->start_time;

                if(cur_seconds >= pthis->time_sec)  //当前时间与开始时间差超过设定时间，则关闭文件，并新建下一个文件，
                {
                   memcpy(head,buffer,HEAD_SIZE);
                   if(head->frame.frame_type == 1)  //如果当前帧是I帧
                   {
                       char *cmd = (char *)malloc(sizeof(char) * 1024 *1024);
                       fclose(pthis->save_file);  //关闭目前正在保存的文件
                       stat(filename,&buf);  //计算文件大小
                       printf("file:%s,size:%d\n",filename,buf.st_size);
                        //将文件大小保存到数据库中
                       sprintf(cmd,"update save set size='%d' where path='%s'",buf.st_size,filename);
                       pthis->cmd_data_callback->cmd = DB_CMD_CALLBACK_NONE;
                       pthis->db_exec(cmd,pthis->cmd_data_callback);
                        //计算目前数据库中已有文件的大小，如果超过设定值，则删除最早的文件
                       sprintf(cmd,"select sum(size) as file_size from save");
                       pthis->cmd_data_callback->cmd = DB_CMD_CALLBACK_SUM;  //文件大小求和
                       pthis->db_exec(cmd,pthis->cmd_data_callback);
                       printf("all file size :%d\n",pthis->cmd_data_callback->data->size);

                       if(pthis->cmd_data_callback->data->size > pthis->disk_size * 1024)  //超过目标空间
                       {
                           //查找最早的记录
                           sprintf(cmd,"select path, min(no) from save");
                           pthis->cmd_data_callback->cmd = DB_CMD_CALLBACK_DELETE;
                           pthis->db_exec(cmd,pthis->cmd_data_callback);
                           //删除文件
                           sprintf(cmd,"/bin/rm -f %s",pthis->cmd_data_callback->data->path);
                           printf("delete:%s\n",cmd);
                           system(cmd);
                            //删除数据库记录
                           sprintf(cmd,"delete from save where no in (select min(no) from save)");
                           pthis->cmd_data_callback->cmd = DB_CMD_CALLBACK_NONE;
                           pthis->db_exec(cmd,pthis->cmd_data_callback);
                       }

                        //新建文件
                       //QString str = pthis->current_time.toString("yyyy-MM-dd_HH-mm-ss"); //设置显示格式
                       char *str_cur_time = (char *)malloc(sizeof(char) * 20);
                       time_to_string(cur_time,str_cur_time);

                       sprintf(filename,"%s_%s.h264",pthis->file_name,str_cur_time);
                       pthis->save_file = fopen(filename,"w");
                       //pthis->start_time = QDateTime::currentDateTime();//获取系统现在的时间
                       //保存到数据库
                       //QString str_time = pthis->start_time.toString("yyyy-MM-dd hh:mm:ss"); //设置显示格式
                       //sprintf(cmd,"insert into save(time,path) values('%s','%s') ",str_time.toLatin1().data(),filename);
                       sprintf(cmd,"insert into save(time,path) values('%s','%s') ",str_cur_time,filename);
                       pthis->cmd_data_callback->cmd = DB_CMD_CALLBACK_NONE;
                       pthis->db_exec(cmd,pthis->cmd_data_callback);
                       pthis->start_time = pthis->current_time;
                       free(str_cur_time);
                       free(cmd);
                   }
                }

            }
            if(pthis->save_file != NULL)
              fwrite(buffer + _head,len,1,pthis->save_file);
        }
        pthread_testcancel();
// #if defined(Q_OS_WIN32)
//         usleep(1000);
// #elif defined(Q_OS_MACX)
//         pthread_yield_np();
// #elif defined(Q_OS_UNIX)
//         pthread_yield();
// #endif
    }
    fclose(pthis->save_file);

}
void QSavefile::search_recode()
{
    char cmd[1024];
    sprintf(cmd,"select * from save");
    cmd_data_callback->cmd = DB_CMD_CALLBACK_SELECT;
    cmd_data_callback->pthis = this;
    db_exec(cmd,cmd_data_callback);
}
//数据库的回调处理函数
//返加符号条件的记录的各列信息： ncolumn:总列数  column_value:列的值  column_name:列的名称
//符合一条记录就会调用一次该函数
int QSavefile::select_info(void *param, int ncolumn, char **column_value, char **column_name)
{
   for(int i = 0;i < ncolumn;i++)
   {
       printf("i%d,name:%s,value:%s\n",i,column_name[i],column_value[i]);
   }
   struct db_cmd_callback_t *cmd_data = (struct db_cmd_callback_t *)param;
   QSavefile *ptr = (QSavefile *)(cmd_data->pthis);
   struct db_cmd_info_t *db_data = new struct db_cmd_info_t();
   switch(cmd_data->cmd)
   {
   case DB_CMD_CALLBACK_SELECT:
       if(column_value[0] != NULL)
            db_data->no = atoi(column_value[0]);
       if(column_value[1] != NULL)
       {
          //db_data->time = QDateTime::fromString(column_value[1],"yyyy-MM-dd hh:mm:ss");
          db_data->time = string_to_time(column_value[1]);

       }
       if(column_value[2] != NULL)
            db_data->size = atol(column_value[2]);
       if(column_value[3] != NULL)
            strcpy(db_data->path,column_value[3]);
       ptr->append_db_list(db_data);
       break;
   case DB_CMD_CALLBACK_SUM:
       cmd_data->data->size = atol(column_value[0]);
       break;
   case DB_CMD_CALLBACK_DELETE:
       strcpy(cmd_data->data->path,column_value[0]);
       break;
   case DB_CMD_CALLBACK_COUNT:
       cmd_data->data->size = atol(column_value[0]);
       break;
   default:
       break;
   }
   free(db_data);
   return 0;
}
//函数库命令执行函数
//command:Sql 命令
//cmd_data->cmd: 命令类型
//cmd_data->data: 单条命令处理结果的回传
void QSavefile::db_exec(char *command,struct db_cmd_callback_t *cmd_data)
{
    int ret = sqlite3_exec(db,command,select_info,cmd_data,NULL);
}
//链表处理，将符号条件的记录，按链表格式存放
void QSavefile::append_db_list(db_cmd_info_t *db_data)
{
    struct db_cmd_list_t *p1;
    struct db_cmd_list_t *new_list = new db_cmd_list_t();
    new_list->data = new struct db_cmd_info_t();
    new_list->data->no = db_data->no;
    strcpy(new_list->data->path,db_data->path);
    new_list->data->size = db_data->size;
    new_list->data->time = db_data->time;
    new_list->next = NULL;
    p1 = head_cmd_list;
    while(p1->next != NULL)
    {
        p1 = p1->next;
    }
    if(p1->next == NULL)
    {
        p1->next = new_list;
    }
}
void QSavefile::delete_db_list(long no)
{
    struct db_cmd_list_t *p1,*p2;
    p1 = head_cmd_list->next;
    p2 = head_cmd_list;
    while(p1 != NULL)
    {
        if(p1->data->no == no)
        {
            p2->next = p1->next;
            break;
        }
        p2 = p1;
        p1 = p1->next;
    }

}
