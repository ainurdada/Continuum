#pragma once

#include <string_view>

#include <reflection/public/object_view.h>

#include <component_edit.h>
#include <ui/draw_context.h>

namespace editor {

class EditorSession;

} // namespace editor

namespace editor::ui {

struct DrawRequest {
    EditorSession& session;
    ComponentEditTarget target;
    engine::reflection::ObjectView rootView;
    engine::reflection::ObjectView view;
    EditInteraction interactionKey;
    const engine::reflection::FieldDescriptor* const fieldDesc;
    bool effectiveReadOnly;
};

class IComponentDrawer {
  public:
    virtual ~IComponentDrawer() = default;
    virtual void drawUntyped(const DrawRequest& data) = 0;
};

template <typename T> class ComponentDrawer : public IComponentDrawer {
  public:
    using ValueType = T;

    virtual void draw(DrawContext<ValueType>& ctx) = 0;

    void drawUntyped(const DrawRequest& data) override final {
        DrawContext<ValueType> ctx(data.session, data.target, data.rootView, data.view, data.interactionKey, data.fieldDesc, data.effectiveReadOnly);
        draw(ctx);
    }
};

} // namespace editor::ui