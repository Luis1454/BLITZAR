/*
 * @file modules/qt/src/panels/control/Disclosure.cpp
 * @brief Implementation of a closed-by-default disclosure section.
 */

#include "panels/control/Disclosure.hpp"
#include <QObject>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace bltzr_qt {

QWidget* buildDisclosure(QWidget* parent, const QString& title, QWidget* content)
{
    auto* section = new QWidget(parent);
    auto* layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* toggle = new QToolButton(section);
    toggle->setText(title);
    toggle->setCheckable(true);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);

    content->setParent(section);
    content->setVisible(false);
    QObject::connect(toggle, &QToolButton::toggled, section, [toggle, content](bool expanded) {
        toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        content->setVisible(expanded);
    });

    layout->addWidget(toggle);
    layout->addWidget(content);
    return section;
}

} // namespace bltzr_qt
