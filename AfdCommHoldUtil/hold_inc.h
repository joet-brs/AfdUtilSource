//#include <Qmproto.h>

void   disp_main_menu();


/*struct QueueEntryStatus{
	DWORDLONG qeh;
	WORD priority;
	DWORD serverUserNum;
	DWORDLONG qehNext;
} ;*/

struct  entry_info
{
	short   index_no;
	char    JobFileSpec[100];
    entry_info *next;
};


typedef  struct {

	short   count_of_entries;
	entry_info  *next;

}list_head;

typedef struct {
	DWORD qh;
	DWORD eh;
} QEH_t;

list_head  head_of_list;

T_PCCQENTRY_HOLD   pccom_entry;
BSCTransmit_t_Hold   bsc_entry;   
char   entry[1024];
QueueEntryStatus_t   stat_blk;
entry_info*  get_new_node();
void  disp_second_menu();
void  reque_entries(BOOL All, short entry_no);
void  que_entry(short  index);
void  purge_entries(short  choice);

entry_info *cur_list_ptr = NULL;
char  server_id[6];
char  entry_typ;
char   queuename[40];


