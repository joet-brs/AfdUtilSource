/****************************************************************************
*                                                                            *                                                                              *		                                                          *                                                                                                                                                        *	
*	             PROPRIETARY PROGRAM MATERIAL	                             *                            
*																			 *                                                       
*This material is proprietary to Momentum Systems  and is not to be		     *                     
*reproduced, used or disclosed except in accordance with program license	 *                     
*or upon written authorization of Momentum Systems , 2 Executive Dr,Suite 10,*
*Moorestown NJ 08057.														 *
*																			 *
*																		     *                                                         	
*COPYRIGHT (C) 1997  MOMENTUM SYSTEMS										 *
*All Rights Reserved							                           	 *
*MOMENTUM PROPRIETARY														 *
*																			 *                                                          
******************************************************************************
*																			 *
*                        DISCLAIMER											 *
*																			 *
*The within information is not intended to be nor should such be			 *
*construed as an affirmation of fact, representation or warranty by			 *
*Momentum Systems  of any type, kind, or character.  The within product		 *
*and the related materials are only furnished pursuant and subject to		 *
*the terms and conditions of a duly executed license agreement.  The only 	 *
*warranties made by Momentum systems with respect to the products described  *
*in this material are set forth in the above furnished mentioned agreement.  *		
*																			 *
*The customer should exercise care to assure that use of the software		 *
*will be in full compliance with laws, rules and regulations of the			 *
*jurisdictions with respect to which it is used.							 *
*																			 *
******************************************************************************


 

****************************************************************************
**																		  **
** Product      :  AFD ING												  **
** Module       :  Afd Comm Hold Utility							      **
**																		  **
** File Name    :  afdhold.cpp										      **
**																		  **
** Purpose      :  Hold utility main module						          **
**																		  **
**Author        :														  **
***************************************************************************/


#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <malloc.h>

#include "Qmproto.h"
#include "include.h"
#include "AfdMisc.h"
#include <Afdconfigproto.h>
#include <Afdlog_debug.h>
#include "CommHoldVidio.h"
#include "hold_inc.h"


/* This utility accepts one command line parameter
   which is the Server Id of the Comm TP for which the entries 
   have to be checked.  It then displays all the entries that are in that queue and asks the user 
   to Requeue selected entries or purge selected entries.  */

extern HANDLE hStdIn;       /* standard input */
extern HANDLE hStdOut;      /* standard output */

char  options[4][40] = {"1. ReQueue All",
                        "2. Selective ReQueue",
						"3. Selective Purge",
						"4. Exit"
};




void main(int argc,char **argv)
{


	// Check if the command line parameter is present. If not display message and quit

	if(argc <= 1)
	{
		printf("Server Id not specified as a Command line argument \n");
		exit(0);
	}

	 
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdIn = GetStdHandle(STD_INPUT_HANDLE);

	// Get the srever id
	if(strcpy_s((char *)server_id, sizeof(server_id), argv[1]))
	{
		printf("Server Id Command line argument too long\n");
		exit(0);
	}
	if((server_id[0] == 'P') || (server_id[0] == 'p'))
		entry_typ = 'P';
	else if((server_id[0] == 'B') || (server_id[0] == 'b'))
		entry_typ = 'B';
	else
	{
		printf("Wromg Server Id \n");
		exit(0);
	}
	disp_main_menu();
    printf("Inside Main return\n");
}


void  disp_main_menu()
{
	
	
	DWORD cTokenValue=0;
	DWORD  erc;
	short  num_lines = 0;
	entry_info *newnode, *start_list;
	BOOL  fSuccess;

	short  base_x =20;
	short  base_y =2;
    DWORDLONG  qeh =0;
	BYTE   count;
	
		

	clrscr();

	// Read entries from the queue one after another
	// Get the Hold QueueName from the Config File

	fSuccess = InitConfigFile(NULL);
	if (!fSuccess) {
		printf("Error opening config file.");
		return;
	}

	fSuccess = GetAfdConfigParam((BYTE *)"AFDMain",(BYTE *)"CommHoldQueueName",(BYTE *)queuename, 34, &cTokenValue, 
									 TRUE);

	erc = EstablishQueueServer(queuename, strlen(queuename), 170, TRUE);
    if(erc)
    {
		printf("Cannot establish connection to Queue \n");
		exit(0);
	}

	gotoyx(base_y, base_x);
	printf("List of ON HOLD Jobs for Server Id %s\n", server_id);


	base_y = 4;
	base_x = 8;
	// Initialise list head
	head_of_list . count_of_entries = 0;
	head_of_list . next = NULL;
    
	// Scan the queue and display the Jobnames.
	while(TRUE)
	{
		//memset(&stat_blk, 0, sizeof(QueueEntryStatus));
       	erc = ReadNextQueueEntry(queuename, strlen(queuename), qeh, &entry,1024, &stat_blk,(WORD)sizeof( QueueEntryStatus_t));
		if(!erc)
		{
	      memcpy(&qeh, (void *) &stat_blk.qehNext, (WORD) sizeof(QEH_t));
          if(entry_typ == 'P')
		  {
			  // Check if the retrieved entry is of type PA
			  if((entry[0] != 'P') && (entry[0] != 'p'))
                 continue;
			  else
				memcpy(&pccom_entry, entry, sizeof(T_PCCQENTRY_HOLD));
		  }
		  else
		  {
			  // It is a Bisync Entry
              if((entry[0] != 'B') && (entry[0] != 'b'))
                 continue;
			  else
			  {
				  //printf("Read the first entry\n");
				memcpy(&bsc_entry, entry, sizeof(BSCTransmit_t_Hold));
			  }
		  }
		}
				
		else
		{
			// There are no more entries
			if(head_of_list.count_of_entries == 0)
			{
				gotoyx(base_y, base_x);
				printf("There are no entries in the Queue");
				
			}
			break;
		}

		// There is an entry store and display the JobName
        newnode = get_new_node();
        head_of_list . count_of_entries++;
        newnode -> index_no = head_of_list . count_of_entries;
        if(entry_typ == 'P')
		{
		   count = pccom_entry.PcCommQ.cbJobFileSpec;
		   memcpy(newnode -> JobFileSpec, (char *)pccom_entry.PcCommQ.JobFileSpec, count);
		   newnode -> JobFileSpec[count] = '\0';

		}
		else
		{
			//printf("Before reading the JobfileSpec\n");
			count = bsc_entry.BiSyncQueue.sbXmtFSpec[0];
		    memcpy(newnode -> JobFileSpec, (char *)&bsc_entry.BiSyncQueue.sbXmtFSpec[1], count);
			newnode -> JobFileSpec[count] = '\0';
		//	printf("After reading the JobFileSpec\n");
		}


		// Connect the newnode to the list
        if( head_of_list . count_of_entries == 1)
		{
			// This is the first entry in the list
			head_of_list.next = newnode;
		}
		else
		{
			// Attach the new node to the previous node
			cur_list_ptr -> next = newnode;
		}
		cur_list_ptr = newnode;

		// display the Jobname on the screen
		gotoyx(base_y, base_x);
		printf("%2d. %s", head_of_list.count_of_entries, newnode -> JobFileSpec);
		//num_lines++;
		if(num_lines == 14)
		{
			base_y = 21;
			gotoyx(base_y, base_x);
			printf("Press any key to Continue");
			getchar();
			num_lines = 0;
			clrscr();
			base_y = 4;
           	base_x = 8;
			gotoyx(base_y, base_x);
		}
		else
		{
			base_y++;
			num_lines++;
		}

    }  

	// If there were some entries displayed, show the options to Requeue and Purge
    if(head_of_list . count_of_entries == 0)
	{
		return;
	}

	base_x = 5;
	base_y = 19;
	gotoyx(base_y, base_x);
	printf("Press any key to continue ");
	getchar();
	disp_second_menu();

	// Free the linked list
   start_list = head_of_list.next;
	while(start_list != NULL)
	{
	//	printf("Freein the list\n");
		newnode = start_list;
		start_list = start_list->next;
		if(newnode != NULL)
			free(newnode);
        
	}
	clrscr();
  //  printf("After Clear\n");
}



void   disp_second_menu()
{
	short  loop_i;
	char   choice;
	short  base_x, base_y;
	short   option;
	//DWORD   option;


	// This routine displays the options available for the use
	clrscr();
    base_x = 15;
	base_y = 5;

	for(loop_i=0;loop_i<4;loop_i++)
	{
		gotoyx(base_y, base_x);
		printf("%s",options[loop_i]);
		base_y++;
	}

	/* Take the users choice */
	base_y = 12;
	gotoyx(base_y, base_x);
	printf("Choice :");

	base_x = base_x + 9;
    gotoyx(base_y, base_x);
	readkey(&choice);

	switch(choice)
	{
	   case  '1' :  // Requeue All the entries
		          reque_entries(TRUE, 0);
				  break;
	   case  '2' : // Reque specified entries
		          // Take one job at a time from the user
		         base_y = base_y +1;
				 base_x = 5;
				 gotoyx(base_y, base_x);
				 printf("Give job Number, 0 for End :");
				 base_x = base_x +30;
		         while(TRUE)
				 {
					  gotoyx(base_y, base_x);
					  printf("       ");
					  gotoyx(base_y, base_x);
                      scanf("%d",&option);
					  if(option == 0)
					  {
						   //printf("Rcvzero\n");
						   break;
					  }
                      // Do a rough check on the choice
					  if(option > head_of_list.count_of_entries)
						  continue;

					  reque_entries(FALSE, option);


				 }
				 //printf("Before break2\n");
				 break;
	   case   '3': // Purge Specified entries
		         base_y = base_y +1;
				 base_x = 5;
				 gotoyx(base_y, base_x);
				 printf("Give job Number, 0 for End :");
				 base_x = base_x +30;
		         while(TRUE)
				 {
                      gotoyx(base_y, base_x);
					  printf("           ");
					  gotoyx(base_y, base_x);
					  scanf("%d",&option);
					  if(option == 0)
						  break;
                      // Do a rough check on the choice
					  if(option > head_of_list.count_of_entries)
						  continue;

    				  purge_entries(option);
				 }
				 break;

	   case   '4' :  return;
	   default  :  return;
		         
	}
	
    return;
}




entry_info*  get_new_node()
{
	entry_info   *new_node;

    new_node = (struct entry_info*) malloc(sizeof(struct entry_info));
    new_node -> next = NULL;

	return(new_node);
}

void  reque_entries(BOOL  All, short  entry_no)
{

	entry_info *start_list;
	
	// If all entries have to be Requed , retrieve all the entries available in
	// the queue and Requeue them
  //  printf("Inside Requeu entries\n");
	start_list = head_of_list.next;
	if(All)
	{
		while(start_list != NULL)
		{
			que_entry(start_list->index_no);
			start_list = start_list->next;
		}
            
	}
	else
		que_entry((short)entry_no);
}


void  que_entry(short  index)
{
    entry_info *start_list;
	DWORD  erc;
	char   dest_qname[34];

	start_list = head_of_list.next;
	// Search for the index
//	printf("Inside Que entry\n");
	while(TRUE)
	{
		if(start_list == NULL)
			return;
		if(start_list->index_no == index)
			break;
		else
			start_list = start_list->next;
	}

    if(entry_typ == 'P')
	{
				memset(&pccom_entry,0, sizeof(T_PCCQENTRY_HOLD));
                erc = MarkKeyedQueueEntry(queuename, strlen(queuename), start_list->JobFileSpec, strlen(start_list->JobFileSpec), (offsetof(T_PCCQENTRY_HOLD, PcCommQ) + offsetof(T_PCCQENTRY,JobFileSpec)),0,0,0,&pccom_entry, sizeof(T_PCCQENTRY_HOLD), &stat_blk, sizeof(QueueEntryStatus_t));
				if(!erc)
				{
			     	strcpy(dest_qname, pccom_entry.OriginQName);

			    	// Put the entry back into the queue

					//Reset the hold fields
                    pccom_entry.PcCommQ.cbHold = 0;
					pccom_entry.PcCommQ.Hold=0;
					erc = (unsigned short)AddQueueEntry(dest_qname, strlen(dest_qname), TRUE, 6,170, &pccom_entry.PcCommQ, sizeof( T_PCCQENTRY), 0,0);
				
				   
                }
	}
	else
	{
				// It is a BisyncEntry
                memset(&bsc_entry,0, sizeof(BSCTransmit_t_Hold));
                erc = MarkKeyedQueueEntry(queuename, strlen(queuename), start_list->JobFileSpec, strlen(start_list->JobFileSpec), (offsetof(BSCTransmit_t_Hold, BiSyncQueue) + offsetof(AfdBiSyncQueue_t,sbXmtFSpec[1])),0,0,0,&bsc_entry, sizeof(BSCTransmit_t_Hold), &stat_blk, sizeof(QueueEntryStatus_t));
			    if(!erc)
				{
					// Reset the hold flag
                    bsc_entry.BiSyncQueue.fHold=0;
			    	strcpy(dest_qname, bsc_entry.OriginQName);
                    erc = (unsigned short)AddQueueEntry(dest_qname, strlen(dest_qname), TRUE, 6,170, &bsc_entry.BiSyncQueue, sizeof( AfdBiSyncQueue_t), 0,0);
				
				}
	}

	if(!erc)
		// Delete the Job from the HOLD queue
		erc = RemoveMarkedQueueEntry(queuename, strlen(queuename), stat_blk.qeh);
	
}


void  purge_entries(short choice)
{
    DWORD  erc;
	entry_info *start_list;
	

	start_list = head_of_list.next;
	// Search for the index
	while(TRUE)
	{
		if(start_list == NULL)
			return;
		if(start_list->index_no == choice)
			break;
		else
			start_list = start_list->next;
	}

	// Just remove this entry from the hold queue
	if(entry_typ == 'P')
	{
				memset(&pccom_entry,0, sizeof(T_PCCQENTRY_HOLD));
                erc = MarkKeyedQueueEntry(queuename, strlen(queuename), start_list->JobFileSpec, strlen(start_list->JobFileSpec), (offsetof(T_PCCQENTRY_HOLD, PcCommQ) + offsetof(T_PCCQENTRY,JobFileSpec)),0,0,0,&pccom_entry, sizeof(T_PCCQENTRY_HOLD), &stat_blk, sizeof(QueueEntryStatus_t));	

	}
	else
	{
				// It is a BisyncEntry
                memset(&bsc_entry,0, sizeof(BSCTransmit_t_Hold));
                erc = MarkKeyedQueueEntry(queuename, strlen(queuename), start_list->JobFileSpec, strlen(start_list->JobFileSpec), (offsetof(BSCTransmit_t_Hold, BiSyncQueue) + offsetof(AfdBiSyncQueue_t,sbXmtFSpec[1])),0,0,0,&bsc_entry, sizeof(BSCTransmit_t_Hold), &stat_blk, sizeof(QueueEntryStatus_t));
	}

	if(!erc)
		// Delete the Job from the HOLD queue
		erc = RemoveMarkedQueueEntry(queuename, strlen(queuename), stat_blk.qeh);
	
    
}