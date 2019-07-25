#ifndef _READ_FILE_H
#define _READ_FILE_H

# include <stdio.h>
# include "Tokenizer.h"
# include "linkedList.h"


typedef struct{
  char *modelName;
  char *packageName;
  LinkedList *port;
  char *useStatement;
  char *componentConformace;
  //attribute *attr;
  //pinMapString *map[];
  //tapPort *taptap[];
  //instructionOpcode *insOp[];
  //registerAccess *register[];
  //boundaryRegister *bscell[];
}BSinfo;


typedef struct {
  char *portName;
  int pinType;
  int bitType;
  int integer1;
  int rangeType;
  int integer2;
}portDesc;

typedef struct{
  FILE *fileHandler;
  char *filename;
  int readLineNo;
  Tokenizer *tokenizer;
} FileTokenizer;

void checkAndSkipCommentLine(FileTokenizer *fileTokenizer);
int compareDescriptionName(char *str);
void BSDL_Parser(BSinfo *bsinfo, FileTokenizer *fileTokenizer);
int checkFileExists(char *file_name);
FileTokenizer *createFileTokenizer(char *filename);
Token *getTokenFromFile(FileTokenizer *fileTokenizer);

void handleComponentNameDesc(BSinfo *bsinfo, FileTokenizer *fileTokenizer);
void handleUseStatementDesc(BSinfo *bsinfo, FileTokenizer *fileTokenizer);
void handleGenericParameterDesc(BSinfo *bsinfo, FileTokenizer *fileTokenizer);
void skipLine(FileTokenizer *fileTokenizer);
int checkVHDLidentifier(char *str);
void freeFileTokenizer(FileTokenizer *tokenizer);
void initBSinfo(BSinfo *bsinfo);
void freeBsInfo(BSinfo *bsinfo);
char *getString(FileTokenizer *fileTokenizer, char *strArr[], int *tokenType, int errorCode, int length);


void checkPinMappingStatement(char *compName, FileTokenizer *fileTokenizer);

#endif // _READ_FILE_H
