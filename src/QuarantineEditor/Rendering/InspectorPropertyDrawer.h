#pragma once
#ifndef INSPECTOR_PROPERTY_DRAWER_H
#define INSPECTOR_PROPERTY_DRAWER_H

#include <string>

struct SerializableComponent;
struct QEMetaField;

namespace InspectorPropertyDrawer
{
    bool IsSupportedInspectorType(const QEMetaField& field);

    bool DrawSerializableObject(
        SerializableComponent* object,
        const std::string& headerLabel,
        const std::string& idPrefix);

    bool DrawField(
        SerializableComponent* object,
        const QEMetaField& field,
        const std::string& widgetId);
}

#endif
