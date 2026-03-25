
#include <windows.h>
#include <winreg.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <QMProto.h>
#include <QMErcs.h>
#include <include.h>

#define MAX_QUEUES 255


afdRespQEntry_t afdrespQ;

typedef struct {
	DWORD qh;
	DWORD eh;
} QEH_t;


BYTE rgQueueFilePrefix[]="[Sys]<SPL>";
BYTE random=0;

QueueInfo_t rgQueues[MAX_QUEUES];

QueueStatus_t *pQMStatus;
QueueInfo_t *pQueue;

BOOL fHexDump = FALSE;

SYSTEMTIME activateTime;

/*-------------------------------------*/
/* sbtos - convert lstring to c string */
/*									   */
/* convert ctos-style sbString to null */
/* terminated string.				   */
/*-------------------------------------*/

char *sbtos(char *s)
{
int i;

	i = *s;								/* string length */
	memmove(s, s+1, i);						/* copy string */
	*(s+i) = '\0';							/* null-terminate */
	return(s);
}

void CheckErc(DWORD erc) {
	printf("Error %8X encountered. Program terminating.", erc);
	exit(erc);
}

void DumpHex(BYTE *p, DWORD c) {

DWORD clines = c/16;
WORD i, j;
BYTE cpri;

	if (!fHexDump)
		return;

	for(i=0;(i<clines);i++) {
	
		printf("%04X  ", i);
		for (j=0;(j<16);j++) {
			printf("%02X ", *(p+j+(i*16)));
		}
		printf("   ");
		for (j=0;(j<16);j++) {
			cpri = *(p+j+(i*16));
			if ((cpri > 0x20) && (cpri < 0x7a))
			;
			else
				cpri = 0x2e;
			printf("%c", cpri);
		}
		printf("\n");

	}

}

void initrgQueues()
{

WORD i, j, rgQueueIndex=0;
DWORD erc;

	pQMStatus = (void *) malloc(4096);
	if (!pQMStatus)
		CheckErc(400);
		
	memset(pQMStatus, 0, 4096);

	/*DWORD GetQMStatus(WORD wQueueType, BOOL fHealthCheck, 
				  LPVOID pStatusRet, WORD sStatusMax);*/

	for(i=0;(i<256);i++) {
		erc = GetQMStatus(i, 0xff, (void *) pQMStatus, 4096);
		if ((!erc) && (pQMStatus->nQueues)) {
			pQueue = (QueueInfo_t *) pQMStatus->rgQueues;
			for(j=0;(j<pQMStatus->nQueues);j++) {
				memmove(&rgQueues[rgQueueIndex++], pQueue++,
						sizeof(QueueInfo_t));
			} /*j++*/
		} /*nQueues*/
	} /*i++*/
	free(pQMStatus);

}

DWORDLONG QEH;

void QMMarkNextQueueEntry()
{

char rgQueueName[51];
DWORD erc=0xffff;
QueueEntryStatus_t qStatus;
char entry[1024];
BOOL fReturn=0xff;
QEH_t *qehCur, *qehNext;

	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("\nBlock if no entries : (Yes/No) : ");
	scanf("%s", entry);
	printf("\n");
	
	fReturn = ( (entry[0] == 'N') || (entry[0] == 'n') );

		/*DWORD MarkNextQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
						 BOOL fReturnIfNoEntries, 
						 LPVOID pEntryRet, WORD sEntryRet, 
						 LPVOID pStatusBlock, WORD sStatusBlock);*/



	/*erc = MarkNextQueueEntry((void *) rgQueueName, (WORD) strlen(rgQueueName),
							 fReturn, (void *) &afdrespQ, sizeof(afdrespQ),
							 (void *) &qStatus, sizeof(QueueEntryStatus_t));*/
	erc = MarkNextQueueEntry((void *) rgQueueName, (WORD) strlen(rgQueueName),
							 fReturn, (void *) &entry, sizeof(entry),
							 (void *) &qStatus, sizeof(QueueEntryStatus_t));
	if (!erc) {
		//entry[80]=0;
		qehCur = (QEH_t *) &qStatus.qeh;
		qehNext = (QEH_t *) &qStatus.qehNext;
		printf("qeh.qh = %4X, qeh.eh = %4X, priority = %2X, serverUserNum = %4X\n",
				qehCur->qh, qehCur->eh, qStatus.priority, qStatus.serverUserNum);
		printf("qehNext.qh = %4X, qehNext.eh = %4X\n", 
				qehNext->qh, qehNext->eh);
		printf("Entry : %s\n", entry);
		memcpy((void *) &QEH, (void *) &qStatus.qeh, 8);

	}
	else
		printf("End of list %4X.\n", erc);

}


void QMMarkKeyedQueueEntry()
{
char rgQueueName[51];
DWORD erc=0xffff;
QueueEntryStatus_t qStatus;
char entry[1024];
QEH_t *qehCur, *qehNext;
WORD oKey1= 0, oKey2 = 0, sKey1 = 0, sKey2 = 0;
BYTE key1[100], key2[100];
void *pKey1 = NULL, *pKey2 = NULL;


	memset(&qStatus, 0, sizeof(QueueEntryStatus_t));
	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("First key desired (Y/N) ? ");
	scanf("%s", &key1);
	if ((key1[0] == 'Y') || (key1[0] == 'y')) {
		printf("Offset of Key1 = ");
		scanf("%d", &oKey1);
		printf("Key 1 is : ");
		scanf("%s", &key1);
		pKey1 = (void *) key1;
		sKey1 = strlen(pKey1);
	} else {
		pKey1 = NULL;
		sKey1 = 0;
	}
	
	printf("Second key desired (Y/N) ? ");
	scanf("%s", &key2);
	if ((key2[0] == 'Y') || (key2[0] == 'y')) {
		printf("Offset of Key2 = ");
		scanf("%d", &oKey2);
		printf("Key 2 is : ");
		scanf("%s", &key2);
		pKey2 = (void *) key2;
		sKey2 = strlen(pKey2);
	} else {
		pKey2 = NULL;
		sKey2 = 0;
	}
	printf("\n");

	/*DWORD MarkKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
						  LPVOID pbKey1, WORD cbKey1, WORD oKey1, 
						  LPVOID pbKey2, WORD cbKey2, WORD oKey2, 
						  LPVOID pEntryRet, WORD sEntryRet, 
						  LPVOID pStatusBlock, WORD sStatusBlock);*/



	erc = MarkKeyedQueueEntry((void *) rgQueueName, (WORD) strlen(rgQueueName),
							  (void *) pKey1, sKey1, oKey1, 
							  (void *) pKey2, sKey2, oKey2,
							  (void *) entry, 1024,
							  (void *) &qStatus, sizeof(QueueEntryStatus_t));
	if (!erc) {
		//entry[80]=0;
		qehCur = (QEH_t *) &qStatus.qeh;
		qehNext = (QEH_t *) &qStatus.qehNext;
		printf("qeh.qh = %4X, qeh.eh = %4X, priority = %2X, serverUserNum = %4X\n",
				qehCur->qh, qehCur->eh, qStatus.priority, qStatus.serverUserNum);
		printf("qehNext.qh = %4X, qehNext.eh = %4X\n", 
				qehNext->qh, qehNext->eh);
		printf("Entry : %s\n", entry);
		memcpy((void *) &QEH, (void *) &qStatus.qeh, 8);

	}
	else
		printf("End of list %4X.\n", erc);
}

void QMRemoveMarkedQueueEntry() {

char rgQueueName[51];
DWORD erc=0xffff;


	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
/*	printf("\nEnter QEH.qh : ");
	scanf("%Ix", &pQEH->qh);
	printf("\nEnter QEH.eh : ");
	scanf("%Ix", &pQEH->eh);
	*/
	printf("\n");

	erc = RemoveMarkedQueueEntry((void *) rgQueueName, (WORD) strlen(rgQueueName), QEH);

	if (erc)
		printf("Error on RemoveMarkedQueueEntry() %4X\n", erc);
	else
		printf("Entry successfully removed.\n");


}

void QMUnmarkQueueEntry() {

char rgQueueName[51];
DWORD erc=0xffff;


	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
/*	printf("\nEnter QEH.qh : ");
	scanf("%Ix", &pQEH->qh);
	printf("\nEnter QEH.eh : ");
	scanf("%Ix", &pQEH->eh);
	*/
	printf("\n");

	/*DWORD UnmarkQueueEntry( LPVOID pbQueueName, WORD cbQueueName, DWORDLONG qeh);*/
	erc = UnmarkQueueEntry((void *) rgQueueName, (WORD) strlen(rgQueueName), QEH);

	if (erc)
		printf("Error on UnmarkQueueEntry() %4X\n", erc);
	else
		printf("Entry successfully unmarked.\n");


}

void QMAddQueueEntry()
{
BYTE rgQueueName[51];
unsigned wQueueType;
BYTE QueueEntry[512];
DWORD erc;
WORD timeDelay;

	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("Queue type : ");
	scanf("%d", &wQueueType);
	printf("Queue entry : ");
	scanf("%s", &QueueEntry);
	printf("Enter time delay in minutes : ");
	scanf("%d", &timeDelay);
	printf("\n");

	/*DWORD AddQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
					BOOL fQueueIfNoServer, WORD priority, WORD queueType, 
					LPVOID pEntry, WORD sEntry, 
					LPVOID pDateTime, WORD repeatTime);*/

	if (timeDelay) {
		GetLocalTime(&activateTime);
		activateTime.wMinute += timeDelay;
		if (activateTime.wMinute > 60) {
			activateTime.wHour++;
			activateTime.wMinute -= 60;
		}
#ifdef CONS_CMD
	memset(&afdrespQ, 0, sizeof(afdrespQ));
	afdrespQ.sbJobFSpec[0]=8;
	memcpy(&afdrespQ.sbJobFSpec[1], "$$CNSCMD", 8);
	afdrespQ.cmdCode=1;

	afdrespQ.sbCnsRspQName[0] = 6;
	memcpy(&afdrespQ.sbCnsRspQName, "AfdAdm", 6);

	erc = AddQueueEntry((void *)rgQueueName, (WORD) strlen(rgQueueName), 
							TRUE, (WORD) 0, (WORD) wQueueType,
							(void *) &afdrespQ, (WORD) sizeof(afdrespQ),
							(void *) &activateTime, 0);
#else
		erc = AddQueueEntry((void *)rgQueueName, (WORD) strlen(rgQueueName), 
							TRUE, (WORD) 0, (WORD) wQueueType,
							(void *) QueueEntry, (WORD) strlen(QueueEntry), 
							(void *) &activateTime, 0);
#endif
	} else {
		erc = AddQueueEntry((void *)rgQueueName, (WORD) strlen(rgQueueName), 
							TRUE, (WORD) 0, (WORD) wQueueType,
							(void *) QueueEntry, (WORD) strlen(QueueEntry), 0, 0);
	}

	if (erc)
		printf("Erc %8X in adding queue entry.\n", erc);

}

void QMEstablishQueueServer()
{
BYTE rgQueueName[51], unique[5];
BOOL fUniqueServer;
WORD wQueueType;
DWORD erc;

	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("Queue type : ");
	scanf("%d", &wQueueType);
	printf("Unique server of this queue (Yes/No) : ");
	scanf("%s", unique);
	printf("\n");
	
	fUniqueServer = ( (unique[0] == 'Y') || (unique[0] == 'y') );
	/*DWORD EstablishQueueServer(LPVOID pbQueueName, WORD cbQueueName, 
						   WORD queueType, BOOL fUniqueServer ) ;*/

	
	erc = EstablishQueueServer((VOID *) rgQueueName, (WORD) strlen(rgQueueName), 
								wQueueType, fUniqueServer);

	if (erc)
		printf("Error %8X in EstablishQueueServer().\n", erc);
}

void QMTerminateQueueServer()
{
BYTE rgQueueName[51];
DWORD erc;

	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("\n");

	/*DWORD TerminateQueueServer(LPVOID pbQueueName, WORD cbQueName);*/
	
	
	erc = TerminateQueueServer((void *) rgQueueName, (WORD) strlen(rgQueueName));

	if (erc)
		printf("Error %8X in TerminateQueueServer().\n", erc);
}

void QMReadNextQueueEntry()
{

BYTE rgQueueName[51];
DWORD erc=0xffff, sEntrySize=0;
DWORDLONG qeh=0;
QueueEntryStatus_t qStatus;
BYTE *entry;
WORD i;
WORD chIn;
QEH_t *qehCur, *qehNext;

	memset(&qStatus, 0, sizeof(QueueEntryStatus_t));
	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("\n");
	for (i=0;(i<MAX_QUEUES);i++) {
		if (rgQueues[i].qh) {
			if (_stricmp(rgQueueName, rgQueues[i].QueueName) == 0) {
				sEntrySize = rgQueues[i].sEntrySize;
				break;
			}
		}
	}
	if (sEntrySize == 0) {
		printf("Cannot find Queue or EntrySize if invalid.\n");
		return;
	}

	entry = (void *) malloc(sEntrySize);
	if (!entry) {
		printf("Error in malloc() %d\n", GetLastError());
		return;
	}
	do {
		/*DWORD ReadNextQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
						 DWORDLONG qeh, 
						 LPVOID pEntryRet, WORD sEntryRet, 
						 LPVOID pStatusBlock, WORD sStatusBlock);*/

		erc = ReadNextQueueEntry((void *) rgQueueName, (WORD) strlen(rgQueueName),
								 qeh, (void *) entry, (WORD) sEntrySize,
								 (void *) &qStatus, (WORD)sizeof(QueueEntryStatus_t));

		if (!erc) {
			qehCur = (QEH_t *) &qStatus.qeh;
			qehNext = (QEH_t *) &qStatus.qehNext;

			printf("qeh.qh = %4X, qeh.eh = %4X, priority = %2X, serverUserNum = %4X\n",
					qehCur->qh, qehCur->eh, qStatus.priority, qStatus.serverUserNum);
			printf("qehNext.qh = %4X, qehNext.eh = %4X\n", 
					qehNext->qh, qehNext->eh);
			DumpHex(entry, sEntrySize);
			printf("Entry : %s\n", entry);
			memcpy((void *) &qeh, (void *) &qStatus.qehNext, sizeof(QEH_t));
			printf("<Press any key for next entry>");
			scanf("%c", &chIn);
		}
	} while (erc == 0);
	
	printf("End of list %4X.\n", erc);

	if (entry)
		free(entry);

}

void QMReadKeyedQueueEntry()
{
BYTE rgQueueName[51];
DWORD erc=0xffff, sEntrySize=0;
QueueEntryStatus_t qStatus;
QEH_t *qehCur, *qehNext;
BYTE *entry;
WORD i, oKey1 = 0, oKey2 = 0, sKey1 = 0, sKey2 = 0;
BYTE key1[100], key2[100];
void *pKey1, *pKey2;
WORD chIn;

	memset(&qStatus, 0, sizeof(QueueEntryStatus_t));
	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("First key desired (Y/N) ? ");
	scanf("%s", &key1);
	if ((key1[0] == 'Y') || (key1[0] == 'y')) {
		printf("Offset of Key1 = ");
		scanf("%d", &oKey1);
		printf("Key 1 is : ");
		scanf("%s", &key1);
		pKey1 = (void *) key1;
		sKey1 = strlen(pKey1);
	} else {
		pKey1 = NULL;
		sKey1 = 0;
	}
	
	printf("Second key desired (Y/N) ? ");
	scanf("%s", &key2);
	if ((key2[0] == 'Y') || (key2[0] == 'y')) {
		printf("Offset of Key2 = ");
		scanf("%d", &oKey2);
		printf("Key 2 is : ");
		scanf("%s", &key2);
		pKey2 = (void *) key2;
		sKey2 = strlen(pKey2);
	} else {
		pKey2 = NULL;
		sKey2 = 0;
	}

	printf("\n");
	for (i=0;(i<MAX_QUEUES);i++) {
		if (rgQueues[i].qh) {
			if (_stricmp(rgQueueName, rgQueues[i].QueueName) == 0) {
				sEntrySize = rgQueues[i].sEntrySize;
				break;
			}
		}
	}
	if (sEntrySize == 0) {
		printf("Cannot find Queue or EntrySize if invalid.\n");
		return;
	}

	entry = (void *) malloc(sEntrySize);
	if (!entry) {
		printf("Error in malloc() %d\n", GetLastError());
		return;
	}
		/*DWORD ReadKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
						  LPVOID pbKey1, WORD cbKey1, WORD oKey1, 
						  LPVOID pbKey2, WORD cbKey2, WORD oKey2, 
						  LPVOID pEntryRet, WORD sEntryRet, 
						  LPVOID pStatusBlock, WORD sStatusBlock);*/
	erc = ReadKeyedQueueEntry((void *) rgQueueName, (WORD) strlen(rgQueueName),
							  pKey1, sKey1, oKey1,
							  pKey2, sKey2, oKey2,
							  (void *) entry, (WORD) sEntrySize,
							  (void *) &qStatus, (WORD)sizeof(QueueEntryStatus_t)
							 );
	if (!erc) {
		qehCur = (QEH_t *) &qStatus.qeh;
		qehNext = (QEH_t *) &qStatus.qehNext;
		printf("qeh.qh = %4X, qeh.eh = %4X, priority = %2X, serverUserNum = %4X\n",
				qehCur->qh, qehCur->eh, qStatus.priority, qStatus.serverUserNum);
		printf("qehNext.qh = %4X, qehNext.eh = %4X\n", 
				qehNext->qh, qehNext->eh);
		DumpHex(entry, sEntrySize);
		printf("Entry : %s\n", entry);
		printf("<Press any key for next entry>");
		scanf("%c", &chIn);
	} else
		printf("Erc %4X on matching Keys\n", erc);
	if (entry)
		free(entry);
}


void QMAddQueue()
{
BYTE rgQueueName[51];
BYTE rgFileName[51];
WORD wEntrySize;
WORD wQueueType;
DWORD qh, erc, i;

	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("Queue type : ");
	scanf("%d", &wQueueType);
	printf("Queue entry size : ");
	scanf("%d", &wEntrySize);
	printf("\n");

	rgFileName[0]=0;
	
	//strcpy(rgFileName, rgQueueFilePrefix);
	//strcat(rgFileName, rgQueueName);

	erc = AddQueue(	(void *) rgQueueName, (WORD) strlen(rgQueueName),
					(void *) rgFileName, (WORD) strlen(rgFileName),
					wEntrySize, wQueueType, (void *) &qh);

	if (erc)
		printf("Erc %8x in adding queue.\n", erc);
	else {
		for(i=0;(i<MAX_QUEUES);i++) {
			if (rgQueues[i].qh == 0)
				break;
		}
		if (i < MAX_QUEUES) {
			rgQueues[i].qh = (HANDLE) qh;
			strcpy(rgQueues[i].QueueName, rgQueueName);
			rgQueues[i].sEntrySize = wEntrySize*512;
		}
	}
}

void QMCleanQueue()
{
BYTE rgQueueName[51];
DWORD erc=0xffff, i;

	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("\n");
	for (i=0;(i<MAX_QUEUES);i++) {
		if (rgQueues[i].qh) {
			if (_stricmp(rgQueueName, rgQueues[i].QueueName) == 0) {
			   erc = CleanQueue((DWORD) rgQueues[i].qh);
			   goto AllDone;
			}
		}
	}
	AllDone:
		if (erc == 0xffff)
			printf("Queue not found.\n");
		else
			if (erc)
				printf("Erc %8X on CleanQueue().\n", erc);
	
}

void QMRemoveQueue()
{
BYTE rgQueueName[51];
DWORD erc=0xffff, i;

	printf("Enter Queue name : ");
	scanf("%s", rgQueueName);
	printf("\n");
	for (i=0;(i<MAX_QUEUES);i++) {
		if (rgQueues[i].qh) {
			if (_stricmp(rgQueueName, rgQueues[i].QueueName) == 0) {
			   erc = RemoveQueue((DWORD) rgQueues[i].qh);
			   if (!erc)
				   rgQueues[i].qh = 0;
			   goto AllDone;
			}
		}
	}
	AllDone:
		if (erc == 0xffff)
			printf("Queue not found.\n");
		else
			if (erc)
				printf("Erc %8X on RemoveQueue().\n", erc);
	
}

void QMGetStatus()
{
WORD i, j;
DWORD erc;

QueueStatus_t *pQueueStatus;
QueueInfo_t *pQInfo;

	pQueueStatus = (void *) malloc(4096);
	if (!pQueueStatus)
		CheckErc(400);

	for(i=0;(i<256);i++) {
		erc = GetQMStatus(i, 0xff, (void *) pQueueStatus, 4096);
		if (erc)
			printf("Error %8X for QueueType %d\n", erc, i);
		else
			if (pQueueStatus->nQueues) {
				printf(
				"QueueType %d. nQueues=%d, cQueueFetch=%d. \n",
				 i, pQueueStatus->nQueues, pQueueStatus->cQueueFetches);
				pQInfo = (QueueInfo_t *) pQueueStatus->rgQueues;
				for(j=0;(j<pQueueStatus->nQueues);j++) {
					printf("qh = %4X Entries = %d, qName = %s\n", 
							pQInfo->qh, pQInfo->cEntries, pQInfo->QueueName);
					pQInfo++;
				} /*j++*/
			} /*nQueues*/
	} /*i++*/
	free(pQueueStatus);

}

void main(void) {
unsigned char opt;

#define O_GetQMStatus 1

#define O_AddQueue 2
#define O_CleanQueue 3
#define O_RemoveQueue 4

#define O_AddQueueEntry 5
#define O_ReadNextQueueEntry 6

#define O_EstablishQueueServer 7
#define O_TerminateQueueServer 8

#define O_MarkNextQueueEntry 9
#define O_RemoveMarkedQueueEntry 10
#define O_UnmarkQueueEntry 11

#define O_ReadKeyedQueueEntry 12
#define O_MarkKeyedQueueEntry 13

#define O_DisableHexDump 90
#define O_EnableHexDump 91

#define O_Exit 99


printf("Invoking QueueManager DLL version : %s\n", GetDLLVersion(NULL));

initrgQueues();

for(;;) {
	printf("\n\n");
	printf(" 1 - GetQMStatus\n");
	
	printf("\n");
	printf(" 2 - AddQueue\n");
	printf(" 3 - CleanQueue\n");
	printf(" 4 - RemoveQueue\n");

	printf("\n");
	printf(" 5 - AddQueueEntry\n");
	printf(" 6 - ReadNextQueueEntry\n");

	printf("\n");
	printf(" 7 - EstablishQueueServer\n");
	printf(" 8 - TerminateQueueServer\n");

	printf("\n");
	printf(" 9 - MarkNextQueueEntry\n");
	printf(" 10- RemoveMarkedQueueEntry\n");
	printf(" 11- UnmarkQueueEntry\n");

	printf("\n");
	printf(" 12- ReadKeyedQueueEntry\n");
	printf(" 13- MarkKeyedQueueEntry\n");

	printf("\n");
	printf(" 90- DisableHexDump\n");
	printf(" 91- EnableHexDump\n");
	printf("\n");
	printf(" 99- Exit\n");

	printf("\n");
	printf("Select option : ");
	scanf("%d", &opt);
	printf("\n");
	random++;
	switch (opt) {

		case O_GetQMStatus:
			QMGetStatus();
			break;

		case O_AddQueue:
			QMAddQueue();
			break;

		case O_CleanQueue:
			QMCleanQueue();
			break;

		case O_RemoveQueue:
			QMRemoveQueue();
			break;
				
		case O_AddQueueEntry:
			QMAddQueueEntry();
			break;

		case O_ReadNextQueueEntry:
			QMReadNextQueueEntry();
			break;

		case O_EstablishQueueServer:
			QMEstablishQueueServer();
			break;

		case O_MarkNextQueueEntry:
			QMMarkNextQueueEntry();
			break;

		case O_UnmarkQueueEntry:
			QMUnmarkQueueEntry();
			break;

		case O_RemoveMarkedQueueEntry:
			QMRemoveMarkedQueueEntry();
			break;

		case O_TerminateQueueServer:
			QMTerminateQueueServer();
			break;

		case O_ReadKeyedQueueEntry:
			QMReadKeyedQueueEntry();
			break;

		case O_MarkKeyedQueueEntry:
			QMMarkKeyedQueueEntry();
			break;

		case O_DisableHexDump:
			fHexDump=FALSE;
			break;

		case O_EnableHexDump:
			fHexDump=TRUE;
			break;

		case O_Exit:
			exit(0);
			break;

		default:
			break;
	}
}

}



