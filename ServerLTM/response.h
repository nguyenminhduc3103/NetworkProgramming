#ifndef RESPONSE_H
#define RESPONSE_H

//Authentication Responses
// Login
#define S_LOGIN_OK      "101"
#define S_LOGIN_NF      "121"
#define S_LOGIN_ERR     "131"
#define S_LOGIN_PERM    "141"
#define S_LOGIN_SRV     "501"

// Register
#define S_REG_OK        "102"
#define S_REG_ERR       "132"
#define S_REG_CFL       "162"
#define S_REG_SRV       "502"

//Project Responses
#define RES_LIST_PROJECT_OK      "103"
#define RES_SEARCH_PROJECT_OK    "104"
#define RES_CREATE_PROJECT_OK    "105"
#define RES_ADD_MEMBER_OK        "106"

#define ERR_PROJECT_VALIDATE     "133"
#define ERR_CREATE_PROJECT_VAL   "135"
#define ERR_ADD_MEMBER_VAL       "136"

#define ERR_PROJECT_PERMISSION   "143"
#define ERR_SEARCH_PERMISSION    "144"
#define ERR_ADD_MEMBER_PERM      "146"

#define ERR_PROJECT_NOT_FOUND    "153"
#define ERR_SEARCH_NOT_FOUND     "154"
#define ERR_USER_NOT_FOUND       "156"

#define ERR_PROJECT_CONFLICT     "165"
#define ERR_ADD_MEMBER_CONFLICT  "166"

#define ERR_PROJECT_SERVER       "503"
#define ERR_SEARCH_PROJECT_SRV   "504"
#define ERR_CREATE_PROJECT_SRV   "505"
#define ERR_ADD_MEMBER_SERVER    "506"

//Task Responses
#define RES_LIST_TASK_OK         "107"
#define RES_CREATE_TASK_OK       "108"
#define RES_ASSIGN_TASK_OK       "109"
#define RES_UPDATE_TASK_OK       "110"
#define RES_COMMENT_TASK_OK      "111"

#define ERR_CREATE_TASK_VAL      "138"
#define ERR_ASSIGN_TASK_VAL      "139"
#define ERR_UPDATE_TASK_VAL      "140"

#define ERR_CREATE_TASK_PERM     "148"
#define ERR_ASSIGN_TASK_PERM     "149"
#define ERR_UPDATE_TASK_PERM     "150"
#define ERR_COMMENT_TASK_PERM   "151"

#define ERR_TASK_NOT_FOUND       "158"
#define ERR_ASSIGN_USER_NOT_IN   "159"

#define ERR_TASK_CONFLICT        "168"

#define ERR_LIST_TASK_SERVER     "507"
#define ERR_CREATE_TASK_SERVER   "508"
#define ERR_ASSIGN_TASK_SERVER   "509"
#define ERR_UPDATE_TASK_SERVER   "510"
#define ERR_COMMENT_TASK_SERVER  "511"

//Member Responses
#define RES_UPDATE_MEMBER_OK     "112"

#define ERR_MEMBER_VALIDATE      "142"
#define ERR_MEMBER_PERMISSION    "152"
#define ERR_MEMBER_SERVER        "512"

#endif
