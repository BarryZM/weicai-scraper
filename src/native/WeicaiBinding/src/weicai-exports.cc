#include <nan.h>
#include "win/WCProcess.h"

#include <direct.h>
#include <TlHelp32.h>

#include <Windows.h>
#include <Psapi.h>//EnumProcesses

HINSTANCE hDLL;

bool GetPrivileges()
{
	// 取得当前进程的[Token](标识)句柄

	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
			return true;
		}
		else {
			return false;
		}
	}
	// 取得调试的[LUID](本地唯一的标识符)值

	TOKEN_PRIVILEGES tokenPrivilege = { 0 };

	TOKEN_PRIVILEGES oldPrivilege = { 0 };

	DWORD dwPrivilegeSize;

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tokenPrivilege.Privileges[0].Luid)) {
		CloseHandle(hToken);
		return false;
	}

	// 设置特权数组的元素个数

	tokenPrivilege.PrivilegeCount = 1;

	// 设置[LUID]的属性值

	tokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// 为当前进程取得DEBUG权限

	if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPrivilege, sizeof(TOKEN_PRIVILEGES), &oldPrivilege, &dwPrivilegeSize)) {
		CloseHandle(hToken);
		return false;
	}
	//程序结束时要记得释放句柄

	//CloseHandle(hToken);
	return true;
}


/*
* 进程Dll注入
*/
void Exp_ProcessDllInject(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if (info.Length() != 3) {
		Nan::ThrowTypeError("必须且仅支持三个参数\n");
		return;
	}
	if (!info[0]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}
	if (!info[1]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}
	if (!info[2]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}

	// 被注入的进程名称
	v8::Local<v8::String> processName = v8::Local<v8::String>::Cast(info[0]);
	v8::String::Utf8Value utfProcessName(processName);

	// 要注入的Dll路径
	v8::Local<v8::String> dllPath = v8::Local<v8::String>::Cast(info[1]);
	v8::String::Utf8Value utfDllPath(dllPath);

	// 要注入的Dll名称
	v8::Local<v8::String> dllName = v8::Local<v8::String>::Cast(info[2]);
	v8::String::Utf8Value utfDllName(dllName);

	BOOL ret = GetPrivileges();
	printf("GetPrivileges -> %d\n", ret);

	// 根据进程名取得进程id
	DWORD dwPid = FindProcessPidByName((LPCSTR)std::string(*utfProcessName).c_str());

	// 注入并返回结果
	info.GetReturnValue().Set(Nan::New((bool)ProcessDllInject(dwPid, (LPCSTR)std::string(*utfDllPath).c_str(), (LPCSTR)std::string(*utfDllName).c_str())));
}


/*
* 进程Dll卸载
*/
void Exp_ProcessDllUninstall(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if (info.Length() != 2) {
		Nan::ThrowTypeError("必须且仅支持两个参数\n");
		return;
	}
	if (!info[0]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}
	if (!info[1]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}

	// 被注入的进程名称
	v8::Local<v8::String> processName = v8::Local<v8::String>::Cast(info[0]);
	v8::String::Utf8Value utfProcessName(processName);

	// 要卸载的Dll名称
	v8::Local<v8::String> dllName = v8::Local<v8::String>::Cast(info[1]);
	v8::String::Utf8Value utfDllName(dllName);

	BOOL ret = GetPrivileges();
	printf("GetPrivileges -> %d\n", ret);

	// 根据进程名取得进程id
	DWORD dwPid = FindProcessPidByName((LPCSTR)std::string(*utfProcessName).c_str());

	// 注入并返回结果
	info.GetReturnValue().Set(Nan::New((bool)ProcessDllUninstall(dwPid, (LPCSTR)std::string(*utfDllName).c_str())));
}

/*
* 检查指定进程名称是否存在
*/
void Exp_CheckProcessExists(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if(info.Length()!=1) {
    	Nan::ThrowTypeError("必须且仅支持一个参数\n");
    	return;
  	}
	if (!info[0]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}
	v8::Local<v8::String> processName = v8::Local<v8::String>::Cast(info[0]);
	v8::String::Utf8Value utfProcessName(processName);
	DWORD dwPid = CheckProcessExists((LPCSTR)std::string(*utfProcessName).c_str());
	info.GetReturnValue().Set(Nan::New((bool)dwPid));
}

/*
* 检查进程是否存在指定的dll模块
*/
void Exp_CheckProcessDllExists(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if (info.Length() != 2) {
		Nan::ThrowTypeError("必须且仅支持两个参数\n");
		return;
	}
	if (!info[0]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}
	if (!info[1]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}

	v8::Local<v8::String> processName = v8::Local<v8::String>::Cast(info[0]);
	v8::String::Utf8Value utfProcessName(processName);

	v8::Local<v8::String> dllName = v8::Local<v8::String>::Cast(info[1]);
	v8::String::Utf8Value utfDllName(dllName);

	// 根据进程名取得进程id
	DWORD dwPid = FindProcessPidByName((LPCSTR)std::string(*utfProcessName).c_str());

	DWORD exists = CheckProcessDllExists(dwPid, (LPCSTR)std::string(*utfDllName).c_str());

	info.GetReturnValue().Set(Nan::New((bool)exists));
}

/*
* 启动控制客户端
*/
void Exp_startCtrlClient(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if (info.Length() != 1) {
		Nan::ThrowTypeError("必须且仅支持一个参数\n");
		return;
	}
	if (!info[0]->IsString()) {
		Nan::ThrowTypeError("参数必须是字符串类型");
		return;
	}

	if (hDLL==NULL) {
		// 要加载的Dll路径
		v8::Local<v8::String> dllPathName = v8::Local<v8::String>::Cast(info[0]);
		v8::String::Utf8Value utfDllPathName(dllPathName);
		hDLL = LoadLibrary((LPCSTR)std::string(*utfDllPathName).c_str());
	}

	info.GetReturnValue().Set(Nan::New((bool)(hDLL!=NULL)));
}

/*
* 发送消息
*/
void Exp_sendCtlMsg(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if (hDLL!=NULL) {
		typedef void(*sendCtlMsg)();
		sendCtlMsg func=(sendCtlMsg)GetProcAddress(hDLL,"sendCtlMsg");
		func();
		info.GetReturnValue().Set(Nan::New(true));
	}else{
		info.GetReturnValue().Set(Nan::New(false));
	}
}

void Init(v8::Local<v8::Object> exports) {
	v8::Local<v8::Context> context = exports->CreationContext();

	exports->Set(context,
		Nan::New("CheckProcessExists").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(Exp_CheckProcessExists)
		->GetFunction(context)
		.ToLocalChecked());

	exports->Set(context,
		Nan::New("CheckProcessDllExists").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(Exp_CheckProcessDllExists)
		->GetFunction(context)
		.ToLocalChecked());

	exports->Set(context,
		Nan::New("ProcessDllInject").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(Exp_ProcessDllInject)
		->GetFunction(context)
		.ToLocalChecked());

	exports->Set(context,
		Nan::New("ProcessDllUninstall").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(Exp_ProcessDllUninstall)
		->GetFunction(context)
		.ToLocalChecked());

	exports->Set(context,
		Nan::New("startCtrlClient").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(Exp_startCtrlClient)
		->GetFunction(context)
		.ToLocalChecked());

	exports->Set(context,
		Nan::New("sendCtlMsg").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(Exp_sendCtlMsg)
		->GetFunction(context)
		.ToLocalChecked());
}

NODE_MODULE(weicaiNative, Init)