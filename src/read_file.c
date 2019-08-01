#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "Token.h"
#include "Error.h"
#include "Common.h"
#include "read_file.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "linkedList.h"
#include "handlePortDescription.h"

char *descriptionName[] = {
  "entity",         //0
  "generic",        //1
  "port",           //2
  "use",            //3
  "attribute",      //4
  "end"             //5
};

char *attributeName[] = {
  "COMPONENT_CONFORMANCE",  //0
  "PIN_MAP",                //1
  "TAP_SCAN_CLOCK",         //2
  "TAP_SCAN_IN",            //3
  "TAP_SCAN_MODE",          //4
  "TAP_SCAN_OUT",           //5
  "TAP_SCAN_RESET",         //6
  "COMPLIANCE_PATTERNS",    //7
  "INSTRUCTION_LENGTH",     //8
  "INSTRUCTION_OPCODE",     //9
  "INSTRUCTION_CAPTURE",    //10
  "IDCODE_REGISTER",        //11
  "REGISTER_ACCESS",        //12
  "BOUNDARY_LENGTH",        //13
  "BOUNDARY_REGISTER",      //14
  "DESIGN_WARNING"          //15
};

char *symbolChar[] = {
  "(",  //0 LEFTPAREN
  ")",  //1 RIGHTPAREN
  ";",  //2 SEMICOLON
  ":",  //3 COLON
  ",",  //4 COMMA
  "-",  //5 DASH
};


void checkAndSkipCommentLine(FileTokenizer *fileTokenizer){
  Token *token;
  token = getTokenFromFile(fileTokenizer);
  if(token->type == TOKEN_OPERATOR_TYPE){
    if(strcmp(token->str,symbolChar[5]) == 0){  //45 in ASCII is '-'
      skipLine(fileTokenizer);
      //return;
    }else{
      throwException(ERR_INVALID_COMMEND_LINE,token,("SUPPOSE TO BE '-' but is %s",token->str));
    }
  }else{
    throwException(ERR_INVALID_COMMEND_LINE,token,"SUPPOSE TO BE '-' but is not");
  }

  freeToken(token);
}

void handleDescSelector(char *str, FileTokenizer *fileTokenizer, BSinfo *bsinfo){
  int i = 0;

  while(descriptionName[i] != NULL){
    if(strcmp(descriptionName[i],str)== 0){
      break;
    }else{
      i++;
    }
  }

  switch (i) {
    case 0:
      handleComponentNameDesc(bsinfo, fileTokenizer);
      break;
    case 1:
      handleGenericParameterDesc(bsinfo,fileTokenizer); //BSINFO
      break;
    case 2:
      handlePortDesc(fileTokenizer,bsinfo->port);
      break;
    case 3:
      handleUseStatementDesc(bsinfo, fileTokenizer);
      break;
    case 4:
      handleAttributeSelector(bsinfo, fileTokenizer);
      break;
    default:
      skipLine(fileTokenizer);
      break;
  }
  return;
}

void handleAttributeSelector(BSinfo *bsinfo, FileTokenizer *fileTokenizer){
  Token *token;
  int i = 0;
  token = getTokenFromFile(fileTokenizer);

  if(token->type != TOKEN_IDENTIFIER_TYPE){
    //will change the errorcode
    throwException(ERR_INVALID_TYPE,NULL,"Invalid Attribute Name!!");
  }

  while(attributeName[i] != NULL){
    if(strcmp(attributeName[i],token->str)== 0){
      break;
    }else{
      i++;
    }
  }

  //throw exception error will change
  switch (i) {
    case 8:
      if(bsinfo->instructionLength != -1){
        throwException(ERR_INVALID_TYPE,NULL,"Instruction Length appear more than one!!");
      }
      bsinfo->instructionLength = handleInstructionAndBoundaryLength(fileTokenizer, ERR_INVALID_TYPE, bsinfo->modelName, 0);
      break;
    case 13:
      if(bsinfo->boundaryLength != -1){
        throwException(ERR_INVALID_TYPE,NULL,"Boundary Length appear more than one!!");
      }
      bsinfo->boundaryLength = handleInstructionAndBoundaryLength(fileTokenizer, ERR_INVALID_TYPE, bsinfo->modelName, 1);
      break;
    default:
      skipLine(fileTokenizer);
      break;
  }
  freeToken(token);
  return;
}

void BSDL_Parser(BSinfo *bsinfo, FileTokenizer *fileTokenizer){
  int i = 0;
  Token *token;
  token = getTokenFromFile(fileTokenizer);

  while(token->type != TOKEN_EOF_TYPE){     //when is not EOF loop it
    if(token->type == TOKEN_NULL_TYPE){ //when it is NULL type at the first token of then line, skip line
      freeToken(token);
      token = getTokenFromFile(fileTokenizer);
      continue;
    }else if(token->type == TOKEN_OPERATOR_TYPE){ //if it is comment line, skip the line
      if (strcmp(token->str,symbolChar[5]) == 0){
        checkAndSkipCommentLine(fileTokenizer);
        //continue;
      }else{  // when the comment Line format not is not fullfill, throw error
        throwException(ERR_INVALID_LINE,token,"Do you mean '-'?");
      }
    }else if(token->type == TOKEN_IDENTIFIER_TYPE){ //if it is identifier, check it and do something
      handleDescSelector(token->str, fileTokenizer, bsinfo);
      freeToken(token);
      token = getTokenFromFile(fileTokenizer);
      continue;
    }else{                                     //else skip the Line
      skipLine(fileTokenizer);
    }
    freeToken(token);
    token = getTokenFromFile(fileTokenizer);
  }
  freeToken(token);
}

//check existing of file
int checkFileExists(char *file_name){
  // F_OK use to test for existence of file.
  if( access( file_name, F_OK ) != -1 ) {
    return 1;
  }
  else{
    return 0;
  }
}

FileTokenizer *createFileTokenizer(char *filename){
  FileTokenizer *fileTokenizer;
  fileTokenizer = (FileTokenizer*)malloc(sizeof(FileTokenizer));

  if (checkFileExists(filename) == 1){
    fileTokenizer->fileHandler = fopen(filename, "r");

    if(fileTokenizer->fileHandler == NULL){
      //throw error for cant open the file
      freeFileTokenizer(fileTokenizer);
      throwException(ERR_FILE_INVALID, NULL, "ERROR!! INVALID FILE!!");
    }

    char line[3000];
    fgets(line,sizeof(line),fileTokenizer->fileHandler);
    fileTokenizer->tokenizer = initTokenizer(line);
  }else{
    throwException(ERR_FILE_NOT_EXIST, NULL, "ERROR!! FILE DOES NOT EXISTS!!");
  }
  fileTokenizer->filename = filename;
  fileTokenizer->readLineNo = 0;
  return fileTokenizer;
}

Token *getTokenFromFile(FileTokenizer *fileTokenizer){
  Token *token;
  char line[3000];

  //tokenizer is null, return invalid token due to it reach End of File
  if (fileTokenizer->tokenizer->str == NULL){
    token = createEndOfFileToken();
    return token;
  }

  token = getToken(fileTokenizer->tokenizer);
  //token->type is NULL, then replace the tokenizer with the next line
  //If next line is EOF, tokenizer = NULL to signal that it reach EOF next getToken
  if(token->type == TOKEN_NULL_TYPE){
    freeTokenizer(fileTokenizer->tokenizer);
    if(fgets(line,sizeof(line),fileTokenizer->fileHandler) != NULL){
      fileTokenizer->tokenizer = initTokenizer(line);
    }else{
      fileTokenizer->tokenizer = initTokenizer(NULL);
    }

    fileTokenizer->readLineNo++;

  }
  return token;
}

//FORMAT: entity <component name> is
void handleComponentNameDesc(BSinfo *bsinfo, FileTokenizer *fileTokenizer){
  if(strlen(bsinfo->modelName) > 0){
    throwException(ERR_COMPONENT_NAME_FORMAT,NULL,"Component name appear more than one!!");
  }

  char *format[3] = {NULL,"is", NULL};
  int tokenType[3] = {8,8,1}; //8->identifier token, 1->NULL token
  int length = sizeof(tokenType)/sizeof(tokenType[0]);
  bsinfo->modelName = getString(fileTokenizer,format,tokenType,ERR_COMPONENT_NAME_FORMAT,length);
}

//FORMAT: use <user package name><period>all<semicolon>
void handleUseStatementDesc(BSinfo *bsinfo, FileTokenizer *fileTokenizer){
  if(strlen(bsinfo->useStatement) > 0){
    throwException(ERR_USE_STATEMENT,NULL,"Use statement appear more than one!!");
  }

  char *format[5] = {NULL,".","all",";",NULL};
  int tokenType[5] = {8,4,8,4,1}; //8->identifier token, 1->NULL token, 4->operator token
  int length = sizeof(tokenType)/sizeof(tokenType[0]);
  bsinfo->useStatement = getString(fileTokenizer,format,tokenType,ERR_USE_STATEMENT,length);
}

void handleGenericParameterDesc(BSinfo *bsinfo,FileTokenizer *fileTokenizer){
  Token *token;
  char *format[10] = {"(","PHYSICAL_PIN_MAP",":","string",":","=",NULL,")",";",NULL};
  int tokenType[10] = {4,8,4,8,4,4,6,4,4,1}; // 1->NULL token, 4->operator token, 6->string token, 8->identifier token
  char *genericParameter;
  genericParameter = "";
  int i = 0;

  while(i < 10){
    token = getTokenFromFile(fileTokenizer);
    if(tokenType[i] == token->type){
      if(i == 9){ //when get the null token, i+1
        i++;
      }else if(i == 6){   //store default device package type into genericParameter
        if(checkVHDLidentifier(token->str) == 0){
          throwException(ERR_GENERIC_PARAMETER,token,"Invalid package name!!");
        }

        if(strlen(bsinfo->packageName) > 0){
          throwException(ERR_GENERIC_PARAMETER,token,"Generic Parameter is call more than one!!");
        }

        genericParameter = (char *)malloc(strlen(token->str));
        strcpy(genericParameter,(token->str));
        i++;
      }else if(strcmp(format[i],token->str)==0){  //compare all string
        i++;
      // if the string of i=5 is not same, it might be generic default type
      }else if((i == 4) && (strcmp(format[i+3],token->str)==0)){
        genericParameter = "";
        i = i + 4;
      }else{
        throwException(ERR_GENERIC_PARAMETER, token, "ERROR!! INVALID GENERIC PARAMETER FORMAT");
      }
    }else{
      if(i == 9 && token->type == TOKEN_OPERATOR_TYPE){
        if(strcmp(token->str,symbolChar[5]) == 0){
          checkAndSkipCommentLine(fileTokenizer);
          i++;
        }
        else{
          throwException(ERR_GENERIC_PARAMETER,token,"Expect null or comment line but it is not!!");
        }
      }else{
        throwException(ERR_GENERIC_PARAMETER, token, "ERROR!! INVALID GENERIC PARAMETER FORMAT");
      }
    }
    freeToken(token);
  }

  bsinfo->packageName =  genericParameter;
}

void skipLine(FileTokenizer *fileTokenizer){
  char line[3000];
  freeTokenizer(fileTokenizer->tokenizer);

  if (fgets(line,sizeof(line),fileTokenizer->fileHandler) != NULL){
    fileTokenizer->tokenizer = initTokenizer(line);
    fileTokenizer->readLineNo++;
    return;
  }else{
    fileTokenizer->tokenizer = initTokenizer(NULL);
    fileTokenizer->readLineNo++;
    return;
  }
}

//check for the VHDL identifier format
// 0: fail, 1 pass
int checkVHDLidentifier(char *str){
  int strLength = strlen(str);
  int underScoreFlag = 0;
  int i = 0;
  char *underscore;
  underscore = "_";

  if (str[strLength-1] == 95){  //95 => in ASCII is '_'
    return 0;
  }

  while(i < strLength){
    if(str[i] == 95){
      i++;
      underScoreFlag++;
    }else if(isalnum(str[i]) != 0){
      i++;
      underScoreFlag = 0;
    }else{
      return 0;
    }

    if (underScoreFlag == 2){
      return 0;
    }
  }

  return 1;
}

void initBSinfo(BSinfo *bsinfo){
  bsinfo->modelName = "";
  bsinfo->packageName = "";
  bsinfo->useStatement = "";
  bsinfo->componentConformace = "";
  bsinfo->instructionLength = -1;
  bsinfo->boundaryLength = -1;
  //bsinfo->port = (LinkedList*)malloc(sizeof(LinkedList));
  bsinfo->port = listInit();
}

//who can use: entity, useStatement
char *getString(FileTokenizer *fileTokenizer, char *strArr[], int *tokenType, int errorCode, int length){
  Token *token;
  char errmsg[100];
  int i = 0;
  char *str;

  while(i < length){
    token = getTokenFromFile(fileTokenizer);
    if(tokenType[i] == token->type){
      if(token->type == TOKEN_IDENTIFIER_TYPE){
        if(strArr[i] == NULL){  //store string when strArr is NULL
          if(checkVHDLidentifier(token->str) == 0){ //check VHDL identifier is  correct or not
            throwException(errorCode,token,"Invalid VHDL Identifier");
          }
          str = (char *)malloc(strlen(token->str));
          strcpy(str,(token->str));
        //compare token->str with strArr[i]. Throw error when not same, else continue
        }else if(strcmp(strArr[i],token->str) != 0){
          sprintf(errmsg,"Expect %s but is %s",strArr[i],token->str);
          throwException(errorCode,token,errmsg);
        }
      //check for the operator type character
      }else if(token->type == TOKEN_OPERATOR_TYPE){
        if(strcmp(strArr[i],token->str) != 0){
          sprintf(errmsg,"Expect %s but is %s",strArr[i],token->str);
          throwException(errorCode,token,errmsg);
        }
      }
    }else{
      if(i == (length - 1) && token->type == TOKEN_OPERATOR_TYPE){
        if(strcmp(token->str,symbolChar[5]) == 0){
          checkAndSkipCommentLine(fileTokenizer);
          i++;
        }
        else{
          throwException(errorCode,token,"Expect null or comment line but it is not!!");
        }
      }
      else{
        throwException(errorCode,token,"Invalid format!!");
      }
    }

    i++;
    freeToken(token);
  }
  return str;
}

//use for Instruction Length and Boundary Length
// type 0: instruction length, type 1: boundary length
//format: attribute <attribute name> of <component name> : entity is <value>;
int handleInstructionAndBoundaryLength(FileTokenizer *fileTokenizer,int errorCode, char *compName, int type){
  Token *token;
  char errmsg[100];
  char *strArr[8] = {"of",compName,":","entity","is",NULL,";",NULL};
  int tokenType[8] = {8,8,4,8,8,3,4,1};
  int length = 8;
  int i = 0;
  int value = -1;

  while(i < length){
    token = getTokenFromFile(fileTokenizer);
    if(tokenType[i] == token->type){
      if(token->type == TOKEN_INTEGER_TYPE && strArr[i] == NULL){
        value = ((IntegerToken*)token)->value;  //get the value
        if(type == 0){
          if(value < 2){
            sprintf(errmsg,"Invalid Length!! Expect greater than or equal to 2 but is %s.",token->str);
            throwException(errorCode,token,errmsg);
          }
        }else if(type == 1){
          if(value < 1){
            sprintf(errmsg,"Invalid Length!! Expect greater than 0 but is %s.",token->str);
            throwException(errorCode,token,errmsg);
          }
        }else{
          sprintf(errmsg,"Invalid type!! Expect 0 or 1 but is %d.",type);
          throwException(errorCode,token,errmsg);
        }
      //compare all the string except for the NULL type token
      }else if(token->type != TOKEN_NULL_TYPE){
        if(strcmp(strArr[i],token->str) != 0){
          sprintf(errmsg,"Expect %s but is %s",strArr[i],token->str);
          throwException(errorCode,token,errmsg);
        }
      }

    }else{
      if(i == (length - 1) && token->type == TOKEN_OPERATOR_TYPE){
        if(strcmp(token->str,symbolChar[5]) == 0){
          checkAndSkipCommentLine(fileTokenizer);
          i++;
        }
        else{
          throwException(errorCode,token,"Expect null or comment line but it is not!!");
        }
      }
      else{
        throwException(errorCode,token,"Invalid format!!");
      }
    }

    i++;
    freeToken(token);
  }
  return value;
}

//PG235/444
// input : bsinfo->modelName , fileTokenizer
void checkPinMappingStatement(char *compName, FileTokenizer *fileTokenizer){
  Token *token;
  char *errmsg;
  int i = 0;
  char *arr[7] = {"of",compName,":","entity","is","PHYSICAL_PIN_MAP",";"};
  int tokenType[7] = {8,8,4,8,8,8,4};

  //compare all the string, throw error if fail
  while(i < 7){
    token = getTokenFromFile(fileTokenizer);
    if(tokenType[i] == token->type){
      if(strcmp(arr[0],token->str) != 0){   //string compare...if not same, throw error
        sprintf(errmsg,"Expect %s but is %s",arr[i],token->str);
        throwException(ERR_INVALID_PIN_MAP_STATEMENT,token,errmsg);
      }
    }else{
      sprintf(errmsg,"Expect %s but is not",arr[i]);
      throwException(ERR_INVALID_PIN_MAP_STATEMENT,NULL,errmsg);
    }
    i++;
    freeToken(token);
  }

  //check for NULL and comment line, if it is not then throw error
  token = getTokenFromFile(fileTokenizer);
  if(token->type != TOKEN_NULL_TYPE && token->type != TOKEN_OPERATOR_TYPE){
    throwException(ERR_INVALID_PIN_MAP_STATEMENT,NULL,"Expect null or comment line");
  }else if(token->type == TOKEN_OPERATOR_TYPE){
    if(strcmp(symbolChar[5],token->str) == 0){
      checkAndSkipCommentLine(fileTokenizer);
    }else{
      throwException(ERR_INVALID_PIN_MAP_STATEMENT,NULL,"Invalid comment line");
    }
  }
  freeToken(token);
}

void freeBsInfo(BSinfo *bsinfo){
  if(bsinfo){
    free(bsinfo);
  }
}

void freeFileTokenizer(FileTokenizer *fileTokenizer) {
  freeTokenizer(fileTokenizer->tokenizer);
  if(fileTokenizer) {
    free(fileTokenizer);
  }
}
