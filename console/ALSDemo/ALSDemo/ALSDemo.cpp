// ALSDemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <sensorsapi.h>
#include <sensors.h>
#include <iostream>

int main()
{
    // 初始化COM库
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // 创建传感器管理器实例
    ISensorManager* pSensorManager = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_ISensorManager, reinterpret_cast<void**>(&pSensorManager));

    if (SUCCEEDED(hr)) {
        // 枚举环境光传感器
        ISensorCollection* pSensorCollection = NULL;
        hr = pSensorManager->GetSensorsByType(SENSOR_TYPE_AMBIENT_LIGHT, &pSensorCollection);

        if (SUCCEEDED(hr)) {
            ULONG count = 0;
            pSensorCollection->GetCount(&count);

            if (count > 0) {
                // 获取第一个环境光传感器
                ISensor* pLightSensor = NULL;
                hr = pSensorCollection->GetAt(0, &pLightSensor);

                if (SUCCEEDED(hr)) {
                    // 读取环境光传感器的数据
                    ISensorDataReport* pSensorDataReport = NULL;
                    hr = pLightSensor->GetData(&pSensorDataReport);

                    if (SUCCEEDED(hr)) {
                        PROPVARIANT varValue;
                        PropVariantInit(&varValue);

                        // 获取光照强度
                        hr = pSensorDataReport->GetSensorValue(SENSOR_DATA_TYPE_LIGHT_LEVEL_LUX, &varValue);

                        if (SUCCEEDED(hr)) {
                            std::cout << "current light level lux: " << varValue.fltVal << " lux" << std::endl;
                        }

                        PropVariantClear(&varValue);
                        pSensorDataReport->Release();
                    }

                    pLightSensor->Release();
                }
            }

            pSensorCollection->Release();
        }

        pSensorManager->Release();
    }

    CoUninitialize();
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
