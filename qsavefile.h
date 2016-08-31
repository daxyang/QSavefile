#ifndef QSAVEFILE_H
#define QSAVEFILE_H

//#include "qsavefile_global.h"
//#include "qslidingwindow.h"
//#include "qslidingwindowconsume.h"
#include "QSlidingWindow.h"
#include "QSlidingWindowConsume.h"
#include "pthread.h"
#include "unistd.h"
#include "sqlite3.h"
//#include "qdatetime.h"


#define   DB_CMD_CALLBACK_NONE    0x00
#define   DB_CMD_CALLBACK_SELECT  0x01
#define   DB_CMD_CALLBACK_SUM     0x02
#define   DB_CMD_CALLBACK_DELETE  0x03
#define   DB_CMD_CALLBACK_COUNT   0x04

struct db_cmd_info_t
{
    int no;
    //QDateTime time;
    time_t time;
    long size;
    char path[1024];
};

struct db_cmd_list_t
{
    struct db_cmd_info_t *data;
    struct db_cmd_list_t *next;
};
struct db_cmd_callback_t
{
    int cmd;
    struct db_cmd_info_t *data;
    void *pthis;
};


//class QSAVEFILESHARED_EXPORT QSavefile
class QSavefile
{

public:
    QSavefile();
    void start_save();
    void bound(QSlidingWindow *slidingwnd);
    void stop_save();
    void db_exec(char *command,struct db_cmd_callback_t *cmd_data);
    void set_save_model(int time_s,char *filename,int save_head_flag);
    void delete_table();
    void set_savedisk_size(long disk_size_kb);
    void search_recode();
    void adduser(int user);

private:
    QSlidingWindow *sliding_window;
    int current_user;
    char *file_name;
    FILE *save_file;

    pthread_t save_pthread_id;

    static void * save_run(void *ptr);
    QSlidingWindowConsume *append_user(int user_no);
    QSlidingWindowConsume *save_consume;
    int savestatus;
    int head_inc;

    sqlite3 *db;


    static int select_info(void *param,int ncolumn,char **column_value,char **column_name);
    int time_sec;

    //QDateTime start_time,current_time;
    time_t start_time,current_time;
    struct db_cmd_list_t *head_cmd_list;
    void append_list(struct db_cmd_info_t *cmd_info);

    struct db_cmd_callback_t *cmd_data_callback;

    long disk_size;

    void append_db_list(struct db_cmd_info_t *db_data);
    void delete_db_list(long no);


};

#endif // QSAVEFILE_H
