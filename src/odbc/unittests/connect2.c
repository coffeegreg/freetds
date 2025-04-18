#include "common.h"

/*
 * Test setting current "catalog" before and after connection using
 * either SQLConnect and SQLDriverConnect
 */

static int failed = 0;

static void init_connect(void);

static void
init_connect(void)
{
	CHKAllocEnv(&odbc_env, "S");
	CHKAllocConnect(&odbc_conn, "S");
}

static void
normal_connect(void)
{
	CHKConnect(T(common_pwd.server), SQL_NTS, T(common_pwd.user), SQL_NTS, T(common_pwd.password), SQL_NTS, "SI");
}

static void
driver_connect(const char *conn_str)
{
	SQLTCHAR tmp[1024];
	SQLSMALLINT len;

	CHKDriverConnect(NULL, T(conn_str), SQL_NTS, tmp, TDS_VECTOR_SIZE(tmp), &len, SQL_DRIVER_NOPROMPT, "SI");
}

static void
check_dbname(const char *dbname)
{
	SQLINTEGER len;
	SQLTCHAR out[512];
	char sql[1024];

	len = sizeof(out);
	CHKGetConnectAttr(SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER) out, sizeof(out), &len, "SI");

	if (strcmp(C(out), dbname) != 0) {
		fprintf(stderr, "Current database (%s) is not %s\n", C(out), dbname);
		failed = 1;
	}

	sprintf(sql, "IF DB_NAME() <> '%s' SELECT 1", dbname);
	CHKAllocStmt(&odbc_stmt, "S");
	odbc_check_no_row(sql);
	SQLFreeStmt(odbc_stmt, SQL_DROP);
	odbc_stmt = SQL_NULL_HSTMT;
}

static void
set_dbname(const char *dbname)
{
	CHKSetConnectAttr(SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER) T(dbname),
			  (SQLINTEGER) strlen(dbname) * sizeof(SQLTCHAR), "SI");
}

TEST_MAIN()
{
	char tmp[1024*3];

	if (odbc_read_login_info())
		exit(1);

	/* try setting db name before connect */
	printf("SQLConnect before 1..\n");
	init_connect();
	set_dbname("master");
	normal_connect();
	check_dbname("master");

	/* check change after connection */
	printf("SQLConnect after..\n");
	set_dbname("tempdb");
	check_dbname("tempdb");

	printf("SQLConnect after not existing..\n");
	strcpy(tmp, "IDontExist");
	CHKSetConnectAttr(SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER) tmp, (SQLINTEGER) strlen(tmp), "E");
	check_dbname("tempdb");

	odbc_disconnect();

	/* try setting db name before connect */
	printf("SQLConnect before 2..\n");
	init_connect();
	set_dbname("tempdb");
	normal_connect();
	check_dbname("tempdb");
	odbc_disconnect();

	/* try connect string with using DSN */
	printf("SQLDriverConnect before 1..\n");
	sprintf(tmp, "DSN=%s;UID=%s;PWD=%s;DATABASE=%s;", common_pwd.server,
		common_pwd.user, common_pwd.password, common_pwd.database);
	init_connect();
	set_dbname("master");
	driver_connect(tmp);
	check_dbname(common_pwd.database);
	odbc_disconnect();

	/* try connect string with using DSN */
	printf("SQLDriverConnect before 2..\n");
	sprintf(tmp, "DSN=%s;UID=%s;PWD=%s;", common_pwd.server, common_pwd.user, common_pwd.password);
	init_connect();
	set_dbname("tempdb");
	driver_connect(tmp);
	check_dbname("tempdb");
	odbc_disconnect();

	if (failed) {
		printf("Some tests failed\n");
		return 1;
	}

	printf("Done.\n");
	return 0;
}
