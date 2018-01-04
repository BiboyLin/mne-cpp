//=============================================================================================================
/**
* @file     abstractmeshtreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    AbstractMeshTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractmeshtreeitem.h"
#include "../common/metatreeitem.h"
#include "../../materials/pervertexphongalphamaterial.h"
#include "../../materials/pervertextessphongalphamaterial.h"
#include "../../materials/shownormalsmaterial.h"
#include "../../3dhelpers/custommesh.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AbstractMeshTreeItem::AbstractMeshTreeItem(QEntity* p3DEntityParent, int iType, const QString& text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
, m_pMaterial(new PerVertexPhongAlphaMaterial)
, m_pTessMaterial(new PerVertexTessPhongAlphaMaterial)
, m_pNormalMaterial(new ShowNormalsMaterial())
, m_pCustomMesh(new CustomMesh())
{
    initItem();
}


//*************************************************************************************************************

QPointer<CustomMesh> AbstractMeshTreeItem::getCustomMesh()
{
    return m_pCustomMesh;
}


//*************************************************************************************************************

void AbstractMeshTreeItem::setVertColor(const QVariant& vertColor)
{
    this->setData(vertColor, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);
}


//*************************************************************************************************************

void AbstractMeshTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Abstract Surface item");

    //Add surface meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    //Material options
    MetaTreeItem* pItemMaterialOptions = new MetaTreeItem(MetaTreeItemTypes::UnknownItem, "Material");
    pItemMaterialOptions->setEditable(false);
    list.clear();
    list << pItemMaterialOptions;
    list << new QStandardItem("The material options");
    this->appendRow(list);

    QString surfaceType("Phong Alpha");
    MetaTreeItem* pItemMaterialType = new MetaTreeItem(MetaTreeItemTypes::MaterialType, surfaceType);
    connect(pItemMaterialType, &MetaTreeItem::dataChanged,
            this, &AbstractMeshTreeItem::onSurfaceMaterialChanged);
    list.clear();
    list << pItemMaterialType;
    list << new QStandardItem(pItemMaterialType->toolTip());
    pItemMaterialOptions->appendRow(list);
    data.setValue(QString(surfaceType));
    pItemMaterialType->setData(data, MetaTreeItemRoles::SurfaceMaterial);
    pItemMaterialType->setData(data, Qt::DecorationRole);

    float fTessInner = 1.0;
    MetaTreeItem *itemTessInner = new MetaTreeItem(MetaTreeItemTypes::SurfaceTessInner, QString("%1").arg(fTessInner));
    connect(itemTessInner, &MetaTreeItem::dataChanged,
            this, &AbstractMeshTreeItem::onSurfaceTessInnerChanged);
    list.clear();
    list << itemTessInner;
    list << new QStandardItem(itemTessInner->toolTip());
    pItemMaterialOptions->appendRow(list);
    data.setValue(fTessInner);
    itemTessInner->setData(data, MetaTreeItemRoles::SurfaceTessInner);

    float fTessOuter = 1.0;
    MetaTreeItem *itemTessOuter = new MetaTreeItem(MetaTreeItemTypes::SurfaceTessOuter, QString("%1").arg(fTessOuter));
    connect(itemTessOuter, &MetaTreeItem::dataChanged,
            this, &AbstractMeshTreeItem::onSurfaceTessOuterChanged);
    list.clear();
    list << itemTessOuter;
    list << new QStandardItem(itemTessOuter->toolTip());
    pItemMaterialOptions->appendRow(list);
    data.setValue(fTessOuter);
    itemTessOuter->setData(data, MetaTreeItemRoles::SurfaceTessOuter);

    float fTriangleScale = 1.0;
    MetaTreeItem *itemTriangleScale = new MetaTreeItem(MetaTreeItemTypes::SurfaceTriangleScale, QString("%1").arg(fTriangleScale));
    connect(itemTriangleScale, &MetaTreeItem::dataChanged,
            this, &AbstractMeshTreeItem::onSurfaceTriangleScaleChanged);
    list.clear();
    list << itemTriangleScale;
    list << new QStandardItem(itemTriangleScale->toolTip());
    pItemMaterialOptions->appendRow(list);
    data.setValue(fTriangleScale);
    itemTriangleScale->setData(data, MetaTreeItemRoles::SurfaceTriangleScale);

    //Init materials
    this->addComponent(m_pMaterial);

    //Init custom mesh
    this->addComponent(m_pCustomMesh);
}


//*************************************************************************************************************

void AbstractMeshTreeItem::setData(const QVariant& value, int role)
{
    Abstract3DTreeItem::setData(value, role);

    switch(role) {
        case Data3DTreeModelItemRoles::SurfaceCurrentColorVert:
            if(m_pCustomMesh) {
                m_pCustomMesh->setColor(value.value<MatrixX3f>());
            }
            break;

        default: // do nothing;
            break;
    }
}


//*************************************************************************************************************

void AbstractMeshTreeItem::onSurfaceTessInnerChanged(const QVariant& fTessInner)
{
    this->setMaterialParameter(fTessInner.toFloat(), "innerTess");
}


//*************************************************************************************************************

void AbstractMeshTreeItem::onSurfaceTessOuterChanged(const QVariant& fTessOuter)
{
    this->setMaterialParameter(fTessOuter.toFloat(), "outerTess");
}


//*************************************************************************************************************

void AbstractMeshTreeItem::onSurfaceTriangleScaleChanged(const QVariant& fTriangleScale)
{
    this->setMaterialParameter(fTriangleScale.toFloat(), "triangleScale");
}


//*************************************************************************************************************

void AbstractMeshTreeItem::onColorChanged(const QVariant& color)
{
    QVariant data;
    MatrixX3f matNewVertColor = createVertColor(this->data(Data3DTreeModelItemRoles::SurfaceVert).value<MatrixX3f>(),
                                                color.value<QColor>());

    data.setValue(matNewVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);

    emit colorOriginChanged(matNewVertColor);
}


//*************************************************************************************************************

void AbstractMeshTreeItem::onSurfaceMaterialChanged(const QVariant& sMaterial)
{
    this->removeComponent(m_pTessMaterial);
    this->removeComponent(m_pMaterial);
    this->removeComponent(m_pNormalMaterial);

    if(sMaterial.toString() == "Phong Alpha") {
        this->addComponent(m_pMaterial);
        if(m_pCustomMesh) {
            m_pCustomMesh->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
        }
    } else if(sMaterial.toString() == "Phong Alpha Tesselation") {
        this->addComponent(m_pTessMaterial);
        if(m_pCustomMesh) {
            m_pCustomMesh->setPrimitiveType(Qt3DRender::QGeometryRenderer::Patches);
        }
    } else if(sMaterial.toString() == "Show normals") {
        this->addComponent(m_pNormalMaterial);
        if(m_pCustomMesh) {
            m_pCustomMesh->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
        }
    }
}


//*************************************************************************************************************

MatrixX3f AbstractMeshTreeItem::createVertColor(const MatrixXf& vertices, const QColor& color) const
{
    MatrixX3f matColor(vertices.rows(),3);

    for(int i = 0; i < matColor.rows(); ++i) {
        matColor(i,0) = color.redF();
        matColor(i,1) = color.greenF();
        matColor(i,2) = color.blueF();
    }

    return matColor;
}
