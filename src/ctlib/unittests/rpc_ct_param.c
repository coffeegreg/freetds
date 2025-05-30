#include "common.h"

#include <freetds/macros.h>

static CS_INT ex_display_dlen(CS_DATAFMT * column);
static CS_RETCODE ex_display_header(CS_INT numcols, CS_DATAFMT columns[]);

typedef struct _ex_column_data
{
	CS_SMALLINT indicator;
	CS_CHAR *value;
	CS_INT valuelen;
}
EX_COLUMN_DATA;

/* Testing: array binding of result set */
TEST_MAIN()
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int verbose = 0;

	CS_RETCODE ret;
	CS_INT res_type;
	CS_INT num_cols;

	CS_CHAR cmdbuf[4096];

	CS_DATAFMT datafmt;
	CS_DATAFMT srcfmt;
	CS_DATAFMT destfmt;
	CS_INT intvar;
	CS_SMALLINT smallintvar;
	CS_FLOAT floatvar;
	CS_MONEY moneyvar;
	CS_BINARY binaryvar;
	CS_BIT bitvar = 1;
	char moneystring[10];
	char rpc_name[15];
	CS_INT destlen;
	CS_INT i;
	CS_INT j;
	CS_INT row_count = 0;
	CS_INT rows_read;
	CS_INT disp_len;
	EX_COLUMN_DATA *coldata;
	CS_DATAFMT *outdatafmt;
	CS_SMALLINT msg_id;



	printf("%s: submit a stored procedure using ct_param \n", __FILE__);
	if (verbose) {
		printf("Trying login\n");
	}
	check_call(try_ctlogin, (&ctx, &conn, &cmd, verbose));
	error_to_stdout = true;

	/* do not test error */
	run_command(cmd, "IF OBJECT_ID('sample_rpc') IS NOT NULL DROP PROCEDURE sample_rpc");

	strcpy(cmdbuf, "create proc sample_rpc (@intparam int, \
        @sintparam smallint output, @floatparam float output, \
        @moneyparam money output,  \
        @dateparam datetime output, @charparam char(20) output, @empty varchar(20) output, \
        @binaryparam    binary(20) output, @bitparam bit) \
        as ");

	strcat(cmdbuf, "select @intparam, @sintparam, @floatparam, @moneyparam, \
        @dateparam, @charparam, @binaryparam \
        select @sintparam = @sintparam + @intparam \
        select @floatparam = @floatparam + @intparam \
        select @moneyparam = @moneyparam + convert(money, @intparam) \
        select @dateparam = getdate() \
        select @charparam = \'The char parameters\' \
        select @empty = \'\' \
        select @binaryparam = @binaryparam \
        print \'This is the message printed out by sample_rpc.\'");

	check_call(run_command, (cmd, cmdbuf));

	/*
	 * Assign values to the variables used for parameter passing.
	 */

	intvar = 2;
	smallintvar = 234;
	floatvar = 0.12;
	binaryvar = (CS_BINARY) 0xff;
	strcpy(rpc_name, "sample_rpc");
	strcpy(moneystring, "300.90");

	/*
	 * Clear and setup the CS_DATAFMT structures used to convert datatypes.
	 */

	memset(&srcfmt, 0, sizeof(CS_DATAFMT));
	srcfmt.datatype = CS_CHAR_TYPE;
	srcfmt.maxlength = (CS_INT) strlen(moneystring);
	srcfmt.precision = 5;
	srcfmt.scale = 2;
	srcfmt.locale = NULL;

	memset(&destfmt, 0, sizeof(CS_DATAFMT));
	destfmt.datatype = CS_MONEY_TYPE;
	destfmt.maxlength = sizeof(CS_MONEY);
	destfmt.precision = 5;
	destfmt.scale = 2;
	destfmt.locale = NULL;

	/*
	 * Convert the string representing the money value
	 * to a CS_MONEY variable. Since this routine does not have the
	 * context handle, we use the property functions to get it.
	 */
	check_call(ct_cmd_props, (cmd, CS_GET, CS_PARENT_HANDLE, &conn, CS_UNUSED, NULL));
	check_call(ct_con_props, (conn, CS_GET, CS_PARENT_HANDLE, &ctx, CS_UNUSED, NULL));
	check_call(cs_convert, (ctx, &srcfmt, (CS_VOID *) moneystring, &destfmt, &moneyvar, &destlen));

	/*
	 * Send the RPC command for our stored procedure.
	 */
	check_call(ct_command, (cmd, CS_RPC_CMD, rpc_name, CS_NULLTERM, CS_NO_RECOMPILE));

	/*
	 * Clear and setup the CS_DATAFMT structure, then pass
	 * each of the parameters for the RPC.
	 */
	memset(&datafmt, 0, sizeof(datafmt));
	strcpy(datafmt.name, "@intparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_INT_TYPE;
	datafmt.maxlength = CS_UNUSED;
	datafmt.status = CS_INPUTVALUE;
	datafmt.locale = NULL;

	check_call(ct_param, (cmd, &datafmt, (CS_VOID *) & intvar, CS_SIZEOF(CS_INT), 0));

	strcpy(datafmt.name, "@sintparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_SMALLINT_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	check_call(ct_param, (cmd, &datafmt, (CS_VOID *) & smallintvar, CS_SIZEOF(CS_SMALLINT), 0));

	strcpy(datafmt.name, "@floatparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_FLOAT_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	check_call(ct_param, (cmd, &datafmt, (CS_VOID *) & floatvar, CS_SIZEOF(CS_FLOAT), 0));

	strcpy(datafmt.name, "@moneyparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_MONEY_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	check_call(ct_param, (cmd, &datafmt, (CS_VOID *) & moneyvar, CS_SIZEOF(CS_MONEY), 0));

	strcpy(datafmt.name, "@dateparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_DATETIME4_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	/*
	 * The datetime variable is filled in by the RPC so pass NULL for
	 * the data, 0 for data length, and -1 for the indicator arguments.
	 */
	check_call(ct_param, (cmd, &datafmt, NULL, 0, -1));
	strcpy(datafmt.name, "@charparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_CHAR_TYPE;
	datafmt.maxlength = 60;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	/*
	 * The character string variable is filled in by the RPC so pass NULL
	 * for the data 0 for data length, and -1 for the indicator arguments.
	 */
	check_call(ct_param, (cmd, &datafmt, NULL, 0, -1));

	strcpy(datafmt.name, "@empty");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_VARCHAR_TYPE;
	datafmt.maxlength = 60;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	/*
	 * The character string variable is filled in by the RPC so pass NULL
	 * for the data 0 for data length, and -1 for the indicator arguments.
	 */
	check_call(ct_param, (cmd, &datafmt, NULL, 0, -1));

	strcpy(datafmt.name, "@binaryparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_BINARY_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	check_call(ct_param, (cmd, &datafmt, (CS_VOID *) & binaryvar, CS_SIZEOF(CS_BINARY), 0));

	strcpy(datafmt.name, "@bitparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_BIT_TYPE;
	datafmt.maxlength = 1;
	datafmt.status = 0;
	datafmt.locale = NULL;

	check_call(ct_param, (cmd, &datafmt, (CS_VOID *) & bitvar, CS_SIZEOF(CS_BIT), 0));

	/*
	 * Send the command to the server
	 */
	check_call(ct_send, (cmd));

	/*
	 * Process the results of the RPC.
	 */
	while ((ret = ct_results(cmd, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {
		case CS_ROW_RESULT:
		case CS_PARAM_RESULT:
		case CS_STATUS_RESULT:
			/*
			 * Print the result header based on the result type.
			 */
			switch ((int) res_type) {
			case CS_ROW_RESULT:
				printf("\nROW RESULTS\n");
				break;

			case CS_PARAM_RESULT:
				printf("\nPARAMETER RESULTS\n");
				break;

			case CS_STATUS_RESULT:
				printf("\nSTATUS RESULTS\n");
				break;
			}
			fflush(stdout);

			/*
			 * All three of these result types are fetchable.
			 * Since the result model for rpcs and rows have
			 * been unified in the New Client-Library, we
			 * will use the same routine to display them
			 */

			/*
			 * Find out how many columns there are in this result set.
			 */
			check_call(ct_res_info, (cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL));

			/*
			 * Make sure we have at least one column
			 */
			if (num_cols <= 0) {
				fprintf(stderr, "ct_res_info(CS_NUMDATA) returned zero columns");
				return 1;
			}

			/*
			 * Our program variable, called 'coldata', is an array of
			 * EX_COLUMN_DATA structures. Each array element represents
			 * one column.  Each array element will re-used for each row.
			 * 
			 * First, allocate memory for the data element to process.
			 */
			coldata = (EX_COLUMN_DATA *) malloc(num_cols * sizeof(EX_COLUMN_DATA));
			if (coldata == NULL) {
				fprintf(stderr, "malloc coldata failed \n");
				return 1;
			}

			outdatafmt = (CS_DATAFMT *) malloc(num_cols * sizeof(CS_DATAFMT));
			if (outdatafmt == NULL) {
				fprintf(stderr, "malloc outdatafmt failed \n");
				return 1;
			}

			for (i = 0; i < num_cols; i++) {
				check_call(ct_describe, (cmd, (i + 1), &outdatafmt[i]));

				outdatafmt[i].maxlength = ex_display_dlen(&outdatafmt[i]) + 1;
				outdatafmt[i].datatype = CS_CHAR_TYPE;
				outdatafmt[i].format = CS_FMT_NULLTERM;

				coldata[i].value = (CS_CHAR *) malloc(outdatafmt[i].maxlength);
				if (coldata[i].value == NULL) {
					fprintf(stderr, "malloc coldata.value failed \n");
					return 1;
				}
				coldata[i].value[0] = 0;

				check_call(ct_bind, (cmd, (i + 1), &outdatafmt[i], coldata[i].value, &coldata[i].valuelen,
					      & coldata[i].indicator));
			}

			ex_display_header(num_cols, outdatafmt);

			while (((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED,
						&rows_read)) == CS_SUCCEED) || (ret == CS_ROW_FAIL)) {
				/*
				 * Increment our row count by the number of rows just fetched.
				 */
				row_count = row_count + rows_read;

				/*
				 * Check if we hit a recoverable error.
				 */
				if (ret == CS_ROW_FAIL) {
					printf("Error on row %d.\n", row_count);
					fflush(stdout);
				}

				/*
				 * We have a row.  Loop through the columns displaying the
				 * column values.
				 */
				for (i = 0; i < num_cols; i++) {
					/*
					 * Display the column value
					 */
					printf("%s", coldata[i].value);
					fflush(stdout);

					/*
					 * If not last column, Print out spaces between this
					 * column and next one.
					 */
					if (i != num_cols - 1) {
						disp_len = ex_display_dlen(&outdatafmt[i]);
						disp_len -= coldata[i].valuelen - 1;
						for (j = 0; j < disp_len; j++) {
							fputc(' ', stdout);
						}
					}
				}
				printf("\n");
				fflush(stdout);
			}

			/*
			 * Free allocated space.
			 */
			for (i = 0; i < num_cols; i++) {
				free(coldata[i].value);
			}
			free(coldata);
			free(outdatafmt);

			/*
			 * We're done processing rows.  Let's check the final return
			 * value of ct_fetch().
			 */
			switch ((int) ret) {
			case CS_END_DATA:
				/*
				 * Everything went fine.
				 */
				printf("All done processing rows.\n");
				fflush(stdout);
				break;

			case CS_FAIL:
				/*
				 * Something terrible happened.
				 */
				fprintf(stderr, "ct_fetch returned CS_FAIL\n");
				return 1;
				break;

			default:
				/*
				 * We got an unexpected return value.
				 */
				fprintf(stderr, "ct_fetch returned %d\n", ret);
				return 1;
				break;

			}
			break;

		case CS_MSG_RESULT:
			check_call(ct_res_info, (cmd, CS_MSGTYPE, (CS_VOID *) & msg_id, CS_UNUSED, NULL));
			printf("ct_result returned CS_MSG_RESULT where msg id = %d.\n", msg_id);
			fflush(stdout);
			break;

		case CS_CMD_SUCCEED:
			/*
			 * This means no rows were returned.
			 */
			break;

		case CS_CMD_DONE:
			/*
			 * Done with result set.
			 */
			break;

		case CS_CMD_FAIL:
			/*
			 * The server encountered an error while
			 * processing our command.
			 */
			fprintf(stderr, "ct_results returned CS_CMD_FAIL.");
			break;

		default:
			/*
			 * We got something unexpected.
			 */
			fprintf(stderr, "ct_results returned unexpected result type.");
			return CS_FAIL;
		}
	}

	/*
	 * We're done processing results. Let's check the
	 * return value of ct_results() to see if everything
	 * went ok.
	 */
	switch ((int) ret) {
	case CS_END_RESULTS:
		/*
		 * Everything went fine.
		 */
		break;

	case CS_FAIL:
		/*
		 * Something failed happened.
		 */
		fprintf(stderr, "ct_results failed.");
		break;

	default:
		/*
		 * We got an unexpected return value.
		 */
		fprintf(stderr, "ct_results returned unexpected result type.");
		break;
	}

	run_command(cmd, "DROP PROCEDURE sample_rpc");

	if (verbose) {
		printf("Trying logout\n");
	}
	check_call(try_ctlogout, (ctx, conn, cmd, verbose));

	return 0;
}

static CS_INT
ex_display_dlen(CS_DATAFMT * column)
{
CS_INT len;

	switch ((int) column->datatype) {
	case CS_CHAR_TYPE:
	case CS_VARCHAR_TYPE:
	case CS_TEXT_TYPE:
	case CS_IMAGE_TYPE:
		len = TDS_MIN(column->maxlength, 1024);
		break;

	case CS_BINARY_TYPE:
	case CS_VARBINARY_TYPE:
		len = TDS_MIN((2 * column->maxlength) + 2, 1024);
		break;

	case CS_BIT_TYPE:
	case CS_TINYINT_TYPE:
		len = 3;
		break;

	case CS_SMALLINT_TYPE:
		len = 6;
		break;

	case CS_INT_TYPE:
		len = 11;
		break;

	case CS_REAL_TYPE:
	case CS_FLOAT_TYPE:
		len = 20;
		break;

	case CS_MONEY_TYPE:
	case CS_MONEY4_TYPE:
		len = 24;
		break;

	case CS_DATETIME_TYPE:
	case CS_DATETIME4_TYPE:
		len = 30;
		break;

	case CS_NUMERIC_TYPE:
	case CS_DECIMAL_TYPE:
		len = (CS_MAX_PREC + 2);
		break;

	default:
		len = 12;
		break;
	}

	return TDS_MAX((CS_INT) (strlen(column->name) + 1), len);
}

static CS_RETCODE
ex_display_header(CS_INT numcols, CS_DATAFMT columns[])
{
CS_INT i;
CS_INT l;
CS_INT j;
CS_INT disp_len;

	fputc('\n', stdout);
	for (i = 0; i < numcols; i++) {
		disp_len = ex_display_dlen(&columns[i]);
		printf("%s", columns[i].name);
		fflush(stdout);
		l = disp_len - (CS_INT) strlen(columns[i].name);
		for (j = 0; j < l; j++) {
			fputc(' ', stdout);
			fflush(stdout);
		}
	}
	fputc('\n', stdout);
	fflush(stdout);
	for (i = 0; i < numcols; i++) {
		disp_len = ex_display_dlen(&columns[i]);
		l = disp_len - 1;
		for (j = 0; j < l; j++) {
			fputc('-', stdout);
		}
		fputc(' ', stdout);
	}
	fputc('\n', stdout);

	return CS_SUCCEED;
}
