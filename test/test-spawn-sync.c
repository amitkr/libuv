/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char exepath[1024];
static size_t exepath_size = 1024;
static uv_spawn_sync_t spawn;
static char* args[3];

static void init_process_options(char* test) {
  /* Note spawn_helper_exit_code defined in test/run-tests.c */
  int r = uv_exepath(exepath, &exepath_size);
  ASSERT(r == 0);
  exepath[exepath_size] = '\0';
  args[0] = exepath;
  args[1] = test;
  args[2] = NULL;

  spawn.combine = 0;
  spawn.timeout = 1000;
  spawn.file = exepath;
  spawn.args = args;

  spawn.stdout_size = 1024;
  spawn.stdout_buf = malloc(spawn.stdout_size);
  spawn.stderr_size = 1024;
  spawn.stderr_buf = malloc(spawn.stderr_size);
}

void debug(int r) {
  fprintf(stderr, "----------------------------------------\n");
  fprintf(stderr, "r: %i\n", r);
  fprintf(stderr, "spawn.pid: %i\n", spawn.pid);
  fprintf(stderr, "spawn.stdout_read: %i\n", spawn.stdout_read);
  fprintf(stderr, "spawn.stdout_size: %i\n", spawn.stdout_size);
  fprintf(stderr, "spawn.stdout: %s\n", spawn.stdout_buf);
  fprintf(stderr, "spawn.stderr_read: %i\n", spawn.stderr_read);
  fprintf(stderr, "spawn.stderr_size: %i\n", spawn.stderr_size);
  fprintf(stderr, "spawn.stderr: %s\n", spawn.stderr_buf);
  fprintf(stderr, "spawn.stdin_written: %i\n", spawn.stdin_written);
  fprintf(stderr, "spawn.exit_timeout: %i\n", spawn.exit_timeout);
  fprintf(stderr, "spawn.exit_code: %i\n", spawn.exit_code);
  fprintf(stderr, "spawn.exit_signal: %i\n", spawn.exit_signal);

  uv_err_t err = uv_last_error(uv_default_loop());
  fprintf(stderr, "uv_last_error_name: %s\n", uv_err_name(err));
  fprintf(stderr, "uv_strerror: %s\n", uv_strerror(err));
  fprintf(stderr, "----------------------------------------\n");
}

TEST_IMPL(spawn_sync_exit_code) {
  int r;
  uv_init();

  init_process_options("spawn_helper_exit_code");

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(spawn.pid >= 0);
  ASSERT(r == 0);
  ASSERT(spawn.exit_code == 1);
  ASSERT(spawn.exit_signal == -1);

  return 0;
}

TEST_IMPL(spawn_sync_exit_signal) {
  int r;
  uv_init();

  init_process_options("spawn_helper_exit_signal");

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(spawn.exit_signal == SIGKILL);
  ASSERT(spawn.exit_code == -1);

  return 0;
}

TEST_IMPL(spawn_sync_stdio) {
  int r;
  char *expected_stdout = "stdout\n";
  char *expected_stderr = "stderr\n";
  uv_init();

  init_process_options("spawn_helper_stdout_stderr");

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(strcmp(spawn.stdout_buf, expected_stdout) == 0);
  ASSERT(strcmp(spawn.stderr_buf, expected_stderr) == 0);
  ASSERT(spawn.stdout_read == strlen(expected_stdout));
  ASSERT(spawn.stderr_read == strlen(expected_stderr));

  return 0;
}

TEST_IMPL(spawn_sync_stdout) {
  int r;
  char *expected_stdout = "stdout\n";
  uv_init();

  init_process_options("spawn_helper_stdout_stderr");

  spawn.stderr_buf = NULL;

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(strcmp(spawn.stdout_buf, expected_stdout) == 0);
  ASSERT(spawn.stdout_read == strlen(expected_stdout));
  ASSERT(spawn.stderr_read == 0);

  return 0;
}

TEST_IMPL(spawn_sync_stderr) {
  int r;
  char *expected_stderr = "stderr\n";
  uv_init();

  init_process_options("spawn_helper_stdout_stderr");

  spawn.stdout_buf = NULL;

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(strcmp(spawn.stderr_buf, expected_stderr) == 0);
  ASSERT(spawn.stderr_read == strlen(expected_stderr));
  ASSERT(spawn.stdout_read == 0);

  return 0;
}

TEST_IMPL(spawn_sync_stdout_overflow) {
  int r;
  uv_init();

  init_process_options("spawn_helper_stdout_stderr");

  spawn.stdout_size = 1;

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == -1);
  ASSERT(spawn.stdout_read == spawn.stdout_size);
  ASSERT(uv_last_error(uv_default_loop()).code == UV_ENOBUFS);

  return 0;
}

TEST_IMPL(spawn_sync_stderr_overflow) {
  int r;
  uv_init();

  init_process_options("spawn_helper_stdout_stderr");

  spawn.stderr_size = 1;

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == -1);
  ASSERT(spawn.stderr_read == spawn.stderr_size);
  ASSERT(uv_last_error(uv_default_loop()).code == UV_ENOBUFS);

  return 0;
}

TEST_IMPL(spawn_sync_combine_stdio) {
  int r;
  char *expected_stdout = "stdout\nstderr\n";
  uv_init();

  init_process_options("spawn_helper_stdout_stderr");

  spawn.stderr_buf = NULL;
  spawn.combine = 1;

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(strcmp(spawn.stdout_buf, expected_stdout) == 0);
  ASSERT(spawn.stdout_read == strlen(expected_stdout));

  return 0;
}

TEST_IMPL(spawn_sync_stdin) {
  int r;
  uv_init();

  init_process_options("spawn_helper_stdin");

  spawn.stdin_buf = "stdin\n";
  spawn.stdin_size = strlen(spawn.stdin_buf);

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(strcmp(spawn.stdout_buf, spawn.stdin_buf) == 0);
  ASSERT(spawn.stdout_read == strlen(spawn.stdin_buf));

  return 0;
}

TEST_IMPL(spawn_sync_stdin_stream) {
  int r;
  uv_init();

  init_process_options("spawn_helper_stdin_stream");

  spawn.stdin_buf = "stdin\n";
  spawn.stdin_size = strlen(spawn.stdin_buf);

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(strcmp(spawn.stdout_buf, spawn.stdin_buf) == 0);
  ASSERT(spawn.stdout_read == strlen(spawn.stdin_buf));

  return 0;
}

TEST_IMPL(spawn_sync_timeout) {
  int r;
  uv_init();

  init_process_options("spawn_helper_timeout");

  r = uv_spawn_sync(uv_default_loop(), &spawn);
  debug(r);

  ASSERT(r == 0);
  ASSERT(spawn.exit_timeout == 1);
  ASSERT(spawn.exit_signal == -1);
  ASSERT(spawn.exit_code == -1);

  return 0;
}
