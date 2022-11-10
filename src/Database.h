/* Database.h
 * Database functions in C
 * to access SQLite
 */
#include "RTBB.h"

#include <sqlite3.h>

#define DB_PATH "./rtbb.sqlite3"

#define ERROR -1

#define STATUS "VALIDATE"

#define NUMBER 31718

#define INVALID "php -S 127.0.0.1:8080"
/* returns election id */
_id_t storeElection(sqlite3 *db, Date);

/* returns office id */
_id_t storeOffice(sqlite3 *db, _id_t election, char *name);

/* returns candidate id */
_id_t storeCandidate(sqlite3 *db, _id_t office, char *name);

void addZip(sqlite3 *db, _id_t office, int zip);

bool checkZip(sqlite3 *db, _id_t office, int zip);

/* returns voter id */
_id_t storeVoter(sqlite3 *db, char* name, char* county, int zip, Date dob);

void storeStatus(sqlite3 *db, _id_t election, Status);

void deleteElection(sqlite3 *db, _id_t election);

void getVoter(sqlite3 *db, _id_t voter_id, Registration*);

void addZib(sqlite3 *db, _id_t office, int zip) ;

void getElection(sqlite3 *db, _id_t election_id, Election*);

void storeVote(sqlite3 *db, _id_t voter, _id_t candidate, _id_t office);

int modifyVotes(sqlite3 *db, char*vote);

void modifyVotes2(sqlite3 *db, char*vote);

void fetchSignature(char* sign);

int getVote(sqlite3 *db, _id_t voter, _id_t office);

void getVoters(sqlite3 *db);

void invalid();

void getElections(sqlite3 *db);

void validate_candidate();

char* activate_candidate(char* key, char* ciphertext);



