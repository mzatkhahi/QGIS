/***************************************************************************
  qgsphongtexturedmaterialwidget.cpp
  --------------------------------------
  Date                 : July 2020
  Copyright            : (C) 2020 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsphongtexturedmaterialwidget.h"

#include "qgsphongtexturedmaterialsettings.h"
#include "qgis.h"

QgsPhongTexturedMaterialWidget::QgsPhongTexturedMaterialWidget( QWidget *parent )
  : QgsMaterialSettingsWidget( parent )
{
  setupUi( this );

  QgsPhongTexturedMaterialSettings defaultMaterial;
  setSettings( &defaultMaterial, nullptr );
  textureScaleSpinBox->setClearValue( 0 );
  textureRotationSpinBox->setClearValue( 0 );

  connect( btnAmbient, &QgsColorButton::colorChanged, this, &QgsPhongTexturedMaterialWidget::changed );
  connect( btnSpecular, &QgsColorButton::colorChanged, this, &QgsPhongTexturedMaterialWidget::changed );
  connect( spinShininess, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &QgsPhongTexturedMaterialWidget::changed );
  connect( textureFile, &QgsImageSourceLineEdit::sourceChanged, this, &QgsPhongTexturedMaterialWidget::changed );
  connect( textureScaleSpinBox, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &QgsPhongTexturedMaterialWidget::changed );
  connect( textureRotationSpinBox, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &QgsPhongTexturedMaterialWidget::changed );
}

QgsMaterialSettingsWidget *QgsPhongTexturedMaterialWidget::create()
{
  return new QgsPhongTexturedMaterialWidget();
}

void QgsPhongTexturedMaterialWidget::setSettings( const QgsAbstractMaterialSettings *settings, QgsVectorLayer * )
{
  const QgsPhongTexturedMaterialSettings *phongMaterial = dynamic_cast< const QgsPhongTexturedMaterialSettings * >( settings );
  if ( !phongMaterial )
    return;
  btnAmbient->setColor( phongMaterial->ambient() );
  btnSpecular->setColor( phongMaterial->specular() );
  spinShininess->setValue( phongMaterial->shininess() );
  textureFile->setSource( phongMaterial->diffuseTexturePath() );
  textureScaleSpinBox->setValue( phongMaterial->textureScale() );
  textureRotationSpinBox->setValue( phongMaterial->textureRotation() );
}

QgsAbstractMaterialSettings *QgsPhongTexturedMaterialWidget::settings()
{
  std::unique_ptr< QgsPhongTexturedMaterialSettings > m = qgis::make_unique< QgsPhongTexturedMaterialSettings >();
  m->setAmbient( btnAmbient->color() );
  m->setSpecular( btnSpecular->color() );
  m->setShininess( spinShininess->value() );
  m->setDiffuseTexturePath( textureFile->source() );
  m->setTextureScale( textureScaleSpinBox->value() );
  m->setTextureRotation( textureRotationSpinBox->value() );
  return m.release();
}

