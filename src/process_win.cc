#include "process.h"
#include <cstring>

#include <iostream> //TODO: remove
using namespace std; //TODO: remove

//Based on the example at https://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx
//Note: on Windows it seems impossible to specify which pipes to use
//Thus, if stdin_h, stdout_h and stderr all are NULL, the out,err,in is sent to the parent process instead
process_id_type Process::open(const std::string &command, const std::string &path) {
  if(use_stdin)
    stdin_fd=std::unique_ptr<file_descriptor_type>(new file_descriptor_type);
  if(read_stdout)
    stdout_fd=std::unique_ptr<file_descriptor_type>(new file_descriptor_type);
  if(read_stderr)
    stderr_fd=std::unique_ptr<file_descriptor_type>(new file_descriptor_type);

  HANDLE g_hChildStd_IN_Rd = NULL;
  HANDLE g_hChildStd_IN_Wr = NULL;
  HANDLE g_hChildStd_OUT_Rd = NULL;
  HANDLE g_hChildStd_OUT_Wr = NULL;
  HANDLE g_hChildStd_ERR_Rd = NULL;
  HANDLE g_hChildStd_ERR_Wr = NULL;

  SECURITY_ATTRIBUTES saAttr;

  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  if(stdin_fd) {
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
      return NULL;
    if(!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
      CloseHandle(g_hChildStd_IN_Rd);
      CloseHandle(g_hChildStd_IN_Wr);
      return NULL;
    }
  }
  if(stdout_fd) {
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Rd);
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Wr);
      return NULL;
    }
    if(!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Rd);
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Wr);
      CloseHandle(g_hChildStd_OUT_Rd);
      CloseHandle(g_hChildStd_OUT_Wr);
      return NULL;
    }
  }
  if(stderr_fd) {
    if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0)) {
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Rd);
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Wr);
      if(stdout_fd) CloseHandle(g_hChildStd_OUT_Rd);
      if(stdout_fd) CloseHandle(g_hChildStd_OUT_Wr);
      return NULL;
    }
    if(!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) {
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Rd);
      if(stdin_fd) CloseHandle(g_hChildStd_IN_Wr);
      if(stdout_fd) CloseHandle(g_hChildStd_OUT_Rd);
      if(stdout_fd) CloseHandle(g_hChildStd_OUT_Wr);
      CloseHandle(g_hChildStd_ERR_Rd);
      CloseHandle(g_hChildStd_ERR_Wr);
      return NULL;
    }
  }
  
  PROCESS_INFORMATION process_info;
  STARTUPINFO siStartInfo;
  
  ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
  
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);
  if(stdin_fd) siStartInfo.hStdInput = g_hChildStd_IN_Rd;
  if(stdout_fd) siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
  if(stderr_fd) siStartInfo.hStdError = g_hChildStd_ERR_Wr;
  if(stdin_fd || stdout_fd || stderr_fd)
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
  
  char* path_ptr;
  if(path=="")
    path_ptr=NULL;
  else {
    path_ptr=new char[path.size()+1];
    std::strcpy(path_ptr, path.c_str());
  }
  char* command_cstr=new char[command.size()+1];
  std::strcpy(command_cstr, command.c_str());
  BOOL bSuccess = CreateProcess(NULL,
                                command_cstr,  // command line
                                NULL,          // process security attributes
                                NULL,          // primary thread security attributes
                                TRUE,          // handles are inherited
                                0,             // creation flags
                                NULL,          // use parent's environment
                                path_ptr,      // use parent's current directory
                                &siStartInfo,  // STARTUPINFO pointer
                                &process_info);  // receives PROCESS_INFORMATION

  if(!bSuccess) {
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    if(stdin_fd) CloseHandle(g_hChildStd_IN_Rd);
    if(stdout_fd) CloseHandle(g_hChildStd_OUT_Wr);
    if(stderr_fd) CloseHandle(g_hChildStd_ERR_Wr);
    return NULL;
  }
  else {
    // Close handles to the child process and its primary thread.
    // Some applications might keep these handles to monitor the status
    // of the child process, for example.

    CloseHandle(process_info.hThread);
    if(stdin_fd) CloseHandle(g_hChildStd_IN_Rd);
    if(stdout_fd) CloseHandle(g_hChildStd_OUT_Wr);
    if(stderr_fd) CloseHandle(g_hChildStd_ERR_Wr);
  }

  if(stdin_fd) *stdin_fd=g_hChildStd_IN_Wr;
  if(stdout_fd) *stdout_fd=g_hChildStd_OUT_Rd;
  if(stderr_fd) *stderr_fd=g_hChildStd_ERR_Rd;
  return process_info.hProcess;
}

void Process::async_read() {
  if(stdout_fd) {
    stdout_thread=std::thread([this](){
      DWORD n;
      char buffer[buffer_size];
      for (;;) {
        BOOL bSuccess = ReadFile(*stdout_fd, static_cast<CHAR*>(buffer), static_cast<DWORD>(buffer_size), &n, NULL);
        if(!bSuccess || n == 0)
          break;
        read_stdout(buffer, static_cast<size_t>(n));
      }
    });
  }
  if(stderr_fd) {
    stderr_thread=std::thread([this](){
      DWORD n;
      char buffer[buffer_size];
      for (;;) {
        BOOL bSuccess = ReadFile(*stderr_fd, static_cast<CHAR*>(buffer), static_cast<DWORD>(buffer_size), &n, NULL);
        if(!bSuccess || n == 0)
          break;
        read_stderr(buffer, static_cast<size_t>(n));
      }
    });
  }
}

int Process::get_exit_code() {
  DWORD exit_code;
  WaitForSingleObject(id, INFINITE);
  GetExitCodeProcess(id, &exit_code);
  CloseHandle(id);
  
  if(stdout_thread.joinable())
    stdout_thread.join();
  if(stderr_thread.joinable())
    stderr_thread.join();
  
  stdin_mutex.lock();
  if(stdin_fd) {
    CloseHandle(*stdin_fd);
    stdin_fd.reset();
  }
  stdin_mutex.unlock();
  if(stdout_fd) {
    CloseHandle(*stdout_fd);
    stdout_fd.reset();
  }
  if(stderr_fd) {
    CloseHandle(*stderr_fd);
    stderr_fd.reset();
  }
  
  return static_cast<int>(exit_code);
}

bool Process::write(const char *bytes, size_t n) {
  stdin_mutex.lock();
  if(stdin_fd) {
    DWORD written;
    BOOL bSuccess=WriteFile(id, bytes, static_cast<DWORD>(n), &written, NULL);
    if(!bSuccess || written==0) {
      stdin_mutex.unlock();
      return false;
    }
    else {
      stdin_mutex.unlock();
      return true;
    }
  }
  stdin_mutex.unlock();
  return false;
}

void Process::kill(process_id_type id, bool force) {
  TerminateProcess(id, 2);
}