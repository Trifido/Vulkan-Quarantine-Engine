#pragma once

#ifndef QESingleton_H
#define QESingleton_H

#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
class QESingleton
{
public:
    // Prohibimos la copia y asignación
    QESingleton(const QESingleton&) = delete;
    QESingleton& operator=(const QESingleton&) = delete;

    static T* getInstance() {
        std::call_once(initFlag, []() {
            instance.reset(new T());
            });
        return instance.get();
    }

    static void ResetInstance()
    {
        if (instance != NULL)
        {
            instance.reset();
        }
    }

protected:
    QESingleton() = default; // Constructor protegido para evitar instanciación directa
    virtual ~QESingleton() = default; // Destructor virtual

private:
    static std::unique_ptr<T> instance;
    static std::once_flag initFlag;
};

// Definición de los miembros estáticos
template <typename T>
std::unique_ptr<T> QESingleton<T>::instance = nullptr;

template <typename T>
std::once_flag QESingleton<T>::initFlag;

#endif // !QESingleton_H
