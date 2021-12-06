/*******************************************************************************
 * Copyright 2013-2021 Aerospike, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include <Python.h>
#include <stdbool.h>

#include <aerospike/as_error.h>
#include <aerospike/as_log.h>

#include "client.h"
#include "conversions.h"
#include "exceptions.h"
#include "log.h"

/*
 * Declare's log level constants.
 */
as_status declare_log_constants(PyObject *aerospike)
{

	// Status to be returned.
	as_status status = AEROSPIKE_OK;

	// Check if aerospike object is present or no.
	if (!aerospike) {
		status = AEROSPIKE_ERR;
		goto exit;
	}

	// Add incidividual constants to aerospike module.
	PyModule_AddIntConstant(aerospike, "LOG_LEVEL_OFF", LOG_LEVEL_OFF);
	PyModule_AddIntConstant(aerospike, "LOG_LEVEL_ERROR", LOG_LEVEL_ERROR);
	PyModule_AddIntConstant(aerospike, "LOG_LEVEL_WARN", LOG_LEVEL_WARN);
	PyModule_AddIntConstant(aerospike, "LOG_LEVEL_INFO", LOG_LEVEL_INFO);
	PyModule_AddIntConstant(aerospike, "LOG_LEVEL_DEBUG", LOG_LEVEL_DEBUG);
	PyModule_AddIntConstant(aerospike, "LOG_LEVEL_TRACE", LOG_LEVEL_TRACE);
exit:
	return status;
}

PyObject *Aerospike_Set_Log_Level(PyObject *parent, PyObject *args,
								  PyObject *kwds)
{
	// Aerospike vaiables
	as_error err;
	as_status status = AEROSPIKE_OK;

	// Python Function Arguments
	PyObject *py_log_level = NULL;

	// Initialise error object.
	as_error_init(&err);

	// Python Function Keyword Arguments
	static char *kwlist[] = {"loglevel", NULL};

	// Python Function Argument Parsing
	if (PyArg_ParseTupleAndKeywords(args, kwds, "O|:setLogLevel", kwlist,
									&py_log_level) == false) {
		return NULL;
	}

	// Type check for incoming parameters
	if (!PyInt_Check(py_log_level)) {
		as_error_update(&err, AEROSPIKE_ERR_PARAM, "Invalid log level");
		goto CLEANUP;
	}

	long lLogLevel = PyInt_AsLong(py_log_level);
	if (lLogLevel == (uint32_t)-1 && PyErr_Occurred()) {
		if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
			as_error_update(&err, AEROSPIKE_ERR_PARAM,
							"integer value exceeds sys.maxsize");
			goto CLEANUP;
		}
	}

	// Invoke C API to set log level
	as_log_set_level((as_log_level)lLogLevel);

CLEANUP:

	// Check error object and act accordingly
	if (err.code != AEROSPIKE_OK) {
		PyObject *py_err = NULL;
		error_to_pyobject(&err, &py_err);
		PyObject *exception_type = raise_exception(&err);
		PyErr_SetObject(exception_type, py_err);
		Py_DECREF(py_err);
		return NULL;
	}

	return PyLong_FromLong(status);
}

volatile int log_counter = 0;

bool log_cb(as_log_level level, const char *func, const char *file,
				   uint32_t line, const char *fmt, ...)
{

	char msg[1024];
	va_list ap;
	
	int counter = __sync_fetch_and_add(&log_counter, 1);

	va_start(ap, fmt);
	vsnprintf(msg, 1024, fmt, ap);
	va_end(ap);

	printf("%d:%d %s\n", getpid(), counter, msg);

	return true;
}

PyObject *Aerospike_Set_Log_Handler(PyObject *parent, PyObject *args, PyObject * kwds)
{
	// Register callback to C-SDK
	as_log_set_callback((as_log_callback) log_cb);

	return PyLong_FromLong(0);
}

void Aerospike_Enable_Default_Logging()
{
	// Invoke C API to set log level
	as_log_set_level((as_log_level)LOG_LEVEL_ERROR);
	// Register callback to C-SDK
	as_log_set_callback((as_log_callback)log_cb);

	return;
}