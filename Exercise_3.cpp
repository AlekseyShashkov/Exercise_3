#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <stdlib.h>

#include <string>
#include <regex>
#include <chrono>
#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/ostreamwrapper.h"

// Структура для удобства формирования записей в JSON.
struct JSONString
{
    char time[80];
    const char *sensor = "WMT700 (COM)";   
    std::string windSpeed;
    std::string windDegree;

    // При создании экземпляра записываем текущее время.
    JSONString()
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::time_t nowClock = std::chrono::system_clock::to_time_t(now);
        std::tm nowTm = *std::localtime(&nowClock);

        strftime(time, 80, "%H:%M:%S%p", &nowTm);
    }
};

void CreateJSON(const JSONString &js, rapidjson::Document &document);
void ReadCOM(const HANDLE &serialPort, rapidjson::Document &document);

int main()
{
    // Инициализируем объект JSON.
    rapidjson::Document document;
    document.SetObject();

    // Открываем порт.
    LPCTSTR portName = TEXT("\\\\.\\COM1");    
    HANDLE serialPort = ::CreateFile(portName,
                                     GENERIC_READ, 
                                     0, 
                                     0,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     nullptr);

    // Настраиваем параметры соединения согласно условию.
    DCB serialPortParams;
    ::ZeroMemory(&serialPortParams, sizeof(serialPortParams));
    serialPortParams.DCBlength = sizeof(serialPortParams);
    serialPortParams.BaudRate  = 2400;
    serialPortParams.ByteSize  = DATABITS_8;
    serialPortParams.StopBits  = ONESTOPBIT;
    serialPortParams.Parity    = NOPARITY;
    SetCommState(serialPort, &serialPortParams);

    // Бесконечный цикл чтения данных.
    while (true) {
        ReadCOM(serialPort, document);
    }

    ::CloseHandle(serialPort);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION: void ReadCOM(const HANDLE &, rapidjson::Document &)
//
// PARAMETERS: [in]  serialPort - последоавательный порт;
//             [out] document   - объект JSON;
//
// RETURN VALUE: нет.
//
// PURPOSE: чтение сообщений поступающих на последовательный порт.
//
// COMMENTS: с помощью регулярного выражения проверяем формат сообщения
//           и, если оно соответствует, создаём экземпляр структуры, в 
//           которую записываем величины скорости ветра и направления ветра.
//
void ReadCOM(const HANDLE &serialPort, rapidjson::Document &document)
{
    const int messageLenght = 15;
    const std::regex regex("\\$([0-9]{2}.[0-9]{2})\\,([0-9]{3}.[0-9]{2})\\r\\n");
    std::smatch match;
    std::string message;

    DWORD size;
    BYTE receivedChar;

    while (true) {
        // Побайтово принимаем и формируем сообщение
        ::ReadFile(serialPort, &receivedChar, sizeof(BYTE), &size, nullptr);
        if (size > 0) {
            message += receivedChar;
        }

        bool isFinished = message.length() == messageLenght;
        bool isMatch = regex_match(message, match, regex);
        if (isFinished && isMatch) {
            JSONString js;
            js.windSpeed  = match[1].str();
            js.windDegree = match[2].str();

            CreateJSON(js, document);
            message.clear();
        }
        if (isFinished && !isMatch) {
            message.clear();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION: void CreateJSON(const JSONString &, rapidjson::Document &)
//
// PARAMETERS: [in]  js         - экземпляр структуры с данными строки;
//             [out] document   - объект JSON;
//
// RETURN VALUE: нет.
//
// PURPOSE: формирование JSON с последующей записью в файл Exercise_3.json.
//
// COMMENTS: для работы с JSON используем rapidjson.
//           https://github.com/Tencent/rapidjson
//
void CreateJSON(const JSONString &js, rapidjson::Document &document)
{
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

    rapidjson::Value value(rapidjson::kObjectType);

    // Добавляем данные в JSON.
    value.SetString(js.time,
                    static_cast<rapidjson::SizeType>(strlen(js.time)),
                    allocator);
    document.AddMember("time", value, allocator);
    value.SetString(js.sensor,
                    static_cast<rapidjson::SizeType>(strlen(js.sensor)),
                    allocator);
    document.AddMember("sensor", value, allocator);
    document.AddMember("windSpeed", atof(js.windSpeed.c_str()), allocator);
    document.AddMember("windDegree", atof(js.windDegree.c_str()), allocator);

    // Создаём файл .json и записываем в него наши данные.
    std::ofstream ofs("Exercise_3.json");
    rapidjson::OStreamWrapper osw(ofs);

    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writerJSON(osw);
    document.Accept(writerJSON);

    ofs.close();

    // Преобразуем документ JSON в строку (для наглядности).
    //rapidjson::StringBuffer strbuf;
    //rapidjson::PrettyWriter<rapidjson::StringBuffer> writerString(strbuf);
    //document.Accept(writerString);

    //const std::string str = strbuf.GetString();
    //std::cout << str << std::endl;
}