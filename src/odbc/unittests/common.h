#include <freetds/utils/test_base.h>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#include <freetds/windows.h>
#include <direct.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>

#include <freetds/sysdep_private.h>
#include <freetds/macros.h>
#include <freetds/replacements.h>
#include <freetds/bool.h>

#ifndef HAVE_SQLLEN
#ifndef SQLULEN
#define SQLULEN SQLUINTEGER
#endif
#ifndef SQLLEN
#define SQLLEN SQLINTEGER
#endif
#endif

#ifndef FREETDS_SRCDIR
#define FREETDS_SRCDIR FREETDS_TOPDIR "/src/odbc/unittests"
#endif

extern HENV odbc_env;
extern HDBC odbc_conn;
extern HSTMT odbc_stmt;
extern int odbc_use_version3;
extern void (*odbc_set_conn_attr)(void);
extern const char *odbc_conn_additional_params;
extern char odbc_err[512];
extern char odbc_sqlstate[6];


int odbc_read_login_info(void);
void odbc_report_error(const char *msg, int line, const char *file);
void odbc_read_error(void);


void odbc_check_cols(int n, int line, const char * file);
void odbc_check_rows(int n, int line, const char * file);
#define ODBC_CHECK_ROWS(n) odbc_check_rows(n, __LINE__, __FILE__)
#define ODBC_CHECK_COLS(n) odbc_check_cols(n, __LINE__, __FILE__)
void odbc_reset_statement_proc(SQLHSTMT *stmt, const char *file, int line);
#define odbc_reset_statement() odbc_reset_statement_proc(&odbc_stmt, __FILE__, __LINE__)
void odbc_check_cursor(void);
void odbc_check_no_row(const char *query);
void odbc_test_skipped(void);

#define ODBC_REPORT_ERROR(msg) odbc_report_error(msg, __LINE__, __FILE__)

SQLRETURN odbc_check_res(const char *file, int line, SQLRETURN rc, SQLSMALLINT handle_type, SQLHANDLE handle, const char *func, const char *res);
#define CHKR(func, params, res) \
	odbc_check_res(__FILE__, __LINE__, (func params), 0, 0, #func, res)
#define CHKR2(func, params, type, handle, res) \
	odbc_check_res(__FILE__, __LINE__, (func params), type, handle, #func, res)

SQLSMALLINT odbc_alloc_handle_err_type(SQLSMALLINT type);

#define CHKAllocConnect(a,res) \
	CHKR2(SQLAllocConnect, (odbc_env,a), SQL_HANDLE_ENV, odbc_env, res)
#define CHKAllocEnv(a,res) \
	CHKR2(SQLAllocEnv, (a), 0, 0, res)
#define CHKAllocStmt(a,res) \
	CHKR2(SQLAllocStmt, (odbc_conn,a), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKAllocHandle(a,b,c,res) \
	CHKR2(SQLAllocHandle, (a,b,c), odbc_alloc_handle_err_type(a), b, res)
#define CHKBindCol(a,b,c,d,e,res) \
	CHKR2(SQLBindCol, (odbc_stmt,a,b,c,d,e), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKBindParameter(a,b,c,d,e,f,g,h,i,res) \
	CHKR2(SQLBindParameter, (odbc_stmt,a,b,c,d,e,f,g,h,i), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKCancel(res) \
	CHKR2(SQLCancel, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKCloseCursor(res) \
	CHKR2(SQLCloseCursor, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKColAttribute(a,b,c,d,e,f,res) \
	CHKR2(SQLColAttribute, (odbc_stmt,a,b,c,d,e,f), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKDescribeCol(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLDescribeCol, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKConnect(a,b,c,d,e,f,res) \
	CHKR2(SQLConnect, (odbc_conn,a,b,c,d,e,f), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKDriverConnect(a,b,c,d,e,f,g,res) \
	CHKR2(SQLDriverConnect, (odbc_conn,a,b,c,d,e,f,g), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKEndTran(a,b,c,res) \
	CHKR2(SQLEndTran, (a,b,c), a, b, res)
#define CHKExecDirect(a,b,res) \
	CHKR2(SQLExecDirect, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKExecute(res) \
	CHKR2(SQLExecute, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKExtendedFetch(a,b,c,d,res) \
	CHKR2(SQLExtendedFetch, (odbc_stmt,a,b,c,d), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKFetch(res) \
	CHKR2(SQLFetch, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKFetchScroll(a,b,res) \
	CHKR2(SQLFetchScroll, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKFreeHandle(a,b,res) \
	CHKR2(SQLFreeHandle, (a,b), a, b, res)
#define CHKFreeStmt(a,res) \
	CHKR2(SQLFreeStmt, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetConnectAttr(a,b,c,d,res) \
	CHKR2(SQLGetConnectAttr, (odbc_conn,a,b,c,d), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKGetDiagRec(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLGetDiagRec, (a,b,c,d,e,f,g,h), a, b, res)
#define CHKGetDiagField(a,b,c,d,e,f,g,res) \
	CHKR2(SQLGetDiagField, (a,b,c,d,e,f,g), a, b, res)
#define CHKGetStmtAttr(a,b,c,d,res) \
	CHKR2(SQLGetStmtAttr, (odbc_stmt,a,b,c,d), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetTypeInfo(a,res) \
	CHKR2(SQLGetTypeInfo, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetData(a,b,c,d,e,res) \
	CHKR2(SQLGetData, (odbc_stmt,a,b,c,d,e), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKMoreResults(res) \
	CHKR2(SQLMoreResults, (odbc_stmt), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKNumResultCols(a,res) \
	CHKR2(SQLNumResultCols, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKParamData(a,res) \
	CHKR2(SQLParamData, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKParamOptions(a,b,res) \
	CHKR2(SQLParamOptions, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKPrepare(a,b,res) \
	CHKR2(SQLPrepare, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKPutData(a,b,res) \
	CHKR2(SQLPutData, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKRowCount(a,res) \
	CHKR2(SQLRowCount, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetConnectAttr(a,b,c,res) \
	CHKR2(SQLSetConnectAttr, (odbc_conn,a,b,c), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKSetCursorName(a,b,res) \
	CHKR2(SQLSetCursorName, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetPos(a,b,c,res) \
	CHKR2(SQLSetPos, (odbc_stmt,a,b,c), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetStmtAttr(a,b,c,res) \
	CHKR2(SQLSetStmtAttr, (odbc_stmt,a,b,c), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKSetStmtOption(a,b,res) \
	CHKR2(SQLSetStmtOption, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
SQLRETURN SQLSetStmtOption_nowarning(SQLHSTMT hstmt, SQLSMALLINT option, SQLULEN param);
#define CHKSetStmtOption_nowarning(a,b,res) \
	CHKR2(SQLSetStmtOption_nowarning, (odbc_stmt,a,b), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKTables(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLTables, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKProcedureColumns(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLProcedureColumns, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKColumns(a,b,c,d,e,f,g,h,res) \
	CHKR2(SQLColumns, (odbc_stmt,a,b,c,d,e,f,g,h), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKGetDescRec(a,b,c,d,e,f,g,h,i,j,res) \
	CHKR2(SQLGetDescRec, (Descriptor,a,b,c,d,e,f,g,h,i,j), SQL_HANDLE_STMT, Descriptor, res)
#define CHKGetDescField(desc,a,b,c,d,e,res) \
	CHKR2(SQLGetDescField, ((desc),a,b,c,d,e), SQL_HANDLE_DESC, (desc), res)
#define CHKSetDescField(desc,a,b,c,d,res) \
	CHKR2(SQLSetDescField, ((desc),a,b,c,d), SQL_HANDLE_DESC, (desc), res)
#define CHKGetInfo(a,b,c,d,res) \
	CHKR2(SQLGetInfo, (odbc_conn,a,b,c,d), SQL_HANDLE_DBC, odbc_conn, res)
#define CHKNumParams(a,res) \
	CHKR2(SQLNumParams, (odbc_stmt,a), SQL_HANDLE_STMT, odbc_stmt, res)
#define CHKDescribeParam(a,b,c,d,e,res) \
	CHKR2(SQLDescribeParam, (odbc_stmt,a,b,c,d,e), SQL_HANDLE_STMT, odbc_stmt, res)

int odbc_connect(void);
int odbc_disconnect(void);
SQLRETURN odbc_command_proc(HSTMT stmt, const char *command, const char *file, int line, const char *res);
#define odbc_command(cmd) odbc_command_proc(odbc_stmt, cmd, __FILE__, __LINE__, "SNo")
#define odbc_command2(cmd, res) odbc_command_proc(odbc_stmt, cmd, __FILE__, __LINE__, res)
SQLRETURN odbc_command_with_result(HSTMT stmt, const char *command);
bool odbc_db_is_microsoft(void);
const char *odbc_db_version(void);
unsigned int odbc_db_version_int(void);
bool odbc_driver_is_freetds(void);
int odbc_tds_version(void);

void odbc_mark_sockets_opened(void);
TDS_SYS_SOCKET odbc_find_last_socket(void);

/**
 * Converts an ODBC result into a string.
 * There is no check on destination length, use a buffer big enough.
 */
void odbc_c2string(char *out, SQLSMALLINT out_c_type, const void *in, size_t in_len);

SQLLEN odbc_to_sqlwchar(SQLWCHAR *dst, const char *src, SQLLEN n);
SQLLEN odbc_from_sqlwchar(char *dst, const SQLWCHAR *src, SQLLEN n);

typedef struct odbc_buf ODBC_BUF;
extern ODBC_BUF *odbc_buf;
void *odbc_buf_add(ODBC_BUF** buf, void *ptr);
void *odbc_buf_get(ODBC_BUF** buf, size_t s);
void odbc_buf_free(ODBC_BUF** buf);
#define ODBC_GET(s)  odbc_buf_get(&odbc_buf, s)
#define ODBC_FREE() odbc_buf_free(&odbc_buf)

SQLWCHAR *odbc_get_sqlwchar(ODBC_BUF** buf, const char *s);
char *odbc_get_sqlchar(ODBC_BUF** buf, SQLWCHAR *s);

#undef T
#ifdef UNICODE
/* char to TCHAR */
#define T(s) odbc_get_sqlwchar(&odbc_buf, (s))
/* TCHAR to char */
#define C(s) odbc_get_sqlchar(&odbc_buf, (s))
#else
#define T(s) ((SQLCHAR*)(s))
#define C(s) ((char*)(s))
#endif

char *odbc_buf_asprintf(ODBC_BUF** buf, const char *fmt, ...);

struct odbc_lookup_int
{
	const char *name;
	int value;
};

int odbc_lookup(const char *name, const struct odbc_lookup_int *table, int def);
const char *odbc_lookup_value(int value, const struct odbc_lookup_int *table, const char *def);
extern struct odbc_lookup_int odbc_sql_c_types[];
extern struct odbc_lookup_int odbc_sql_types[];

void odbc_swap_stmts(SQLHSTMT *a, SQLHSTMT *b);
#define SWAP_STMT(stmt) odbc_swap_stmts(&odbc_stmt, &stmt)
