/***************************************************************************
                         qgsalgorithmlayoutatlastoimage.cpp
                         ---------------------
    begin                : June 2020
    copyright            : (C) 2020 by Mathieu Pellerin
    email                : nirvn dot asia at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsalgorithmlayoutatlastoimage.h"
#include "qgslayout.h"
#include "qgslayoutatlas.h"
#include "qgslayoutitemmap.h"
#include "qgsprintlayout.h"
#include "qgsprocessingoutputs.h"
#include "qgslayoutexporter.h"

#include <QImageWriter>

///@cond PRIVATE

QString QgsLayoutAtlasToImageAlgorithm::name() const
{
  return QStringLiteral( "atlaslayouttoimage" );
}

QString QgsLayoutAtlasToImageAlgorithm::displayName() const
{
  return QObject::tr( "Export atlas layout as image" );
}

QStringList QgsLayoutAtlasToImageAlgorithm::tags() const
{
  return QObject::tr( "layout,atlas,composer,composition,save,png,jpeg,jpg" ).split( ',' );
}

QString QgsLayoutAtlasToImageAlgorithm::group() const
{
  return QObject::tr( "Cartography" );
}

QString QgsLayoutAtlasToImageAlgorithm::groupId() const
{
  return QStringLiteral( "cartography" );
}

QString QgsLayoutAtlasToImageAlgorithm::shortDescription() const
{
  return QObject::tr( "Exports an atlas layout as a set of image." );
}

QString QgsLayoutAtlasToImageAlgorithm::shortHelpString() const
{
  return QObject::tr( "This algorithm outputs an atlas layout to a set of image files (e.g. PNG or JPEG images).\n\n"
                      "If a coverage layer is set, the selected layout's atlas settings exposed in this algorithm "
                      "will be overwritten. In this case, an empty filter or sort by expressing will turn those "
                      "settings off." );
}

void QgsLayoutAtlasToImageAlgorithm::initAlgorithm( const QVariantMap & )
{
  addParameter( new QgsProcessingParameterLayout( QStringLiteral( "LAYOUT" ), QObject::tr( "Atlas layout" ) ) );

  addParameter( new QgsProcessingParameterVectorLayer( QStringLiteral( "COVERAGE_LAYER" ), QObject::tr( "Coverage layer" ), QList< int >(), QVariant(), true ) );
  addParameter( new QgsProcessingParameterExpression( QStringLiteral( "FILTER_EXPRESSION" ), QObject::tr( "Filter expression" ), QString(), QStringLiteral( "COVERAGE_LAYER" ), true ) );
  addParameter( new QgsProcessingParameterExpression( QStringLiteral( "SORTBY_EXPRESSION" ), QObject::tr( "Sort expression" ), QString(), QStringLiteral( "COVERAGE_LAYER" ), true ) );
  addParameter( new QgsProcessingParameterBoolean( QStringLiteral( "SORTBY_REVERSE" ), QObject::tr( "Reverse sort order (used when a sort expression is provided)" ), false, true ) );

  addParameter( new QgsProcessingParameterExpression( QStringLiteral( "FILENAME_EXPRESSION" ), QObject::tr( "Output filename expression" ), QStringLiteral( "'output_'||@atlas_featurenumber" ), QStringLiteral( "COVERAGE_LAYER" ) ) );
  addParameter( new QgsProcessingParameterFile( QStringLiteral( "FOLDER" ), QObject::tr( "Output folder" ), QgsProcessingParameterFile::Folder ) );


  std::unique_ptr< QgsProcessingParameterMultipleLayers > layersParam = qgis::make_unique< QgsProcessingParameterMultipleLayers>( QStringLiteral( "LAYERS" ), QObject::tr( "Map layers to assign to unlocked map item(s)" ), QgsProcessing::TypeMapLayer, QVariant(), true );
  layersParam->setFlags( layersParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( layersParam.release() );

  QStringList imageFormats;
  const auto supportedImageFormats { QImageWriter::supportedImageFormats() };
  for ( const auto format : QImageWriter::supportedImageFormats() )
  {
    if ( format == QByteArray( "svg" ) )
      continue;
    imageFormats << QString( format );
  }
  std::unique_ptr< QgsProcessingParameterEnum > extensionParam = qgis::make_unique< QgsProcessingParameterEnum >( QStringLiteral( "EXTENSION" ), QObject::tr( "Image format" ), imageFormats, false, imageFormats.indexOf( QStringLiteral( "png" ) ) );
  extensionParam->setFlags( extensionParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( extensionParam.release() );

  std::unique_ptr< QgsProcessingParameterNumber > dpiParam = qgis::make_unique< QgsProcessingParameterNumber >( QStringLiteral( "DPI" ), QObject::tr( "DPI (leave blank for default layout DPI)" ), QgsProcessingParameterNumber::Double, QVariant(), true, 0 );
  dpiParam->setFlags( dpiParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( dpiParam.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > appendGeorefParam = qgis::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "GEOREFERENCE" ), QObject::tr( "Generate world file" ), true );
  appendGeorefParam->setFlags( appendGeorefParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( appendGeorefParam.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > exportRDFParam = qgis::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "INCLUDE_METADATA" ), QObject::tr( "Export RDF metadata (title, author, etc.)" ), true );
  exportRDFParam->setFlags( exportRDFParam->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( exportRDFParam.release() );

  std::unique_ptr< QgsProcessingParameterBoolean > antialias = qgis::make_unique< QgsProcessingParameterBoolean >( QStringLiteral( "ANTIALIAS" ), QObject::tr( "Enable antialiasing" ), true );
  antialias->setFlags( antialias->flags() | QgsProcessingParameterDefinition::FlagAdvanced );
  addParameter( antialias.release() );
}

QgsProcessingAlgorithm::Flags QgsLayoutAtlasToImageAlgorithm::flags() const
{
  return QgsProcessingAlgorithm::flags() | FlagNoThreading;
}

QgsLayoutAtlasToImageAlgorithm *QgsLayoutAtlasToImageAlgorithm::createInstance() const
{
  return new QgsLayoutAtlasToImageAlgorithm();
}

QVariantMap QgsLayoutAtlasToImageAlgorithm::processAlgorithm( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  // this needs to be done in main thread, layouts are not thread safe
  QgsPrintLayout *layout = parameterAsLayout( parameters, QStringLiteral( "LAYOUT" ), context );
  if ( !layout )
    throw QgsProcessingException( QObject::tr( "Cannot find layout with name \"%1\"" ).arg( parameters.value( QStringLiteral( "LAYOUT" ) ).toString() ) );

  QgsLayoutAtlas *atlas = layout->atlas();

  QgsVectorLayer *previousCoverageLayer = atlas->coverageLayer();
  const bool previousEnabled = atlas->enabled();
  const QString previousFilenameExpression = atlas->filenameExpression();
  const QString previousFilterExpression = atlas->filterExpression();
  const bool previousSortFeatures = atlas->sortFeatures();
  const bool previousSortAscending = atlas->sortAscending();
  const QString previousSortExpression = atlas->sortExpression();
  QStringList previousMapItemLayersReset;

  auto restoreAtlas = [ layout, atlas,
                                previousCoverageLayer,
                                previousEnabled,
                                previousFilenameExpression,
                                previousFilterExpression,
                                previousSortFeatures,
                                previousSortAscending,
                                previousSortExpression,
                                previousMapItemLayersReset ]()
  {
    QString error;
    atlas->setEnabled( previousEnabled );
    atlas->setCoverageLayer( previousCoverageLayer );
    atlas->setFilenameExpression( previousFilenameExpression, error );
    atlas->setFilterExpression( previousFilterExpression, error );
    atlas->setSortFeatures( previousSortFeatures );
    atlas->setSortAscending( previousSortAscending );
    atlas->setSortExpression( previousSortExpression );

    if ( previousMapItemLayersReset.size() > 0 )
    {
      const QList<QGraphicsItem *> items = layout->items();
      for ( QGraphicsItem *graphicsItem : items )
      {
        QgsLayoutItem *item = dynamic_cast<QgsLayoutItem *>( graphicsItem );
        QgsLayoutItemMap *map = dynamic_cast<QgsLayoutItemMap *>( item );
        if ( map && previousMapItemLayersReset.contains( map->uuid() ) )
        {
          map->setLayers( QList<QgsMapLayer *>() );
        }
      }
    }
  };

  QString expression, error;
  QgsVectorLayer *layer = parameterAsVectorLayer( parameters, QStringLiteral( "COVERAGE_LAYER" ), context );
  if ( layer )
  {
    atlas->setEnabled( true );
    atlas->setCoverageLayer( layer );

    expression = parameterAsString( parameters, QStringLiteral( "FILTER_EXPRESSION" ), context );
    atlas->setFilterExpression( expression, error );
    if ( !expression.isEmpty() && !error.isEmpty() )
    {
      restoreAtlas();
      throw QgsProcessingException( QObject::tr( "Error setting atlas filter expression" ) );
    }
    error.clear();

    expression = parameterAsString( parameters, QStringLiteral( "SORTBY_EXPRESSION" ), context );
    if ( !expression.isEmpty() )
    {
      const bool sortByReverse = parameterAsBool( parameters, QStringLiteral( "SORTBY_REVERSE" ), context );
      atlas->setSortFeatures( true );
      atlas->setSortExpression( expression );
      atlas->setSortAscending( !sortByReverse );
    }
    else
    {
      atlas->setSortFeatures( false );
    }
  }
  else if ( !atlas->enabled() )
  {
    throw QgsProcessingException( QObject::tr( "Layout being export doesn't have an enabled atlas" ) );
  }

  expression = parameterAsString( parameters, QStringLiteral( "FILENAME_EXPRESSION" ), context );
  atlas->setFilenameExpression( expression, error );
  if ( !error.isEmpty() )
  {
    restoreAtlas();
    throw QgsProcessingException( QObject::tr( "Error setting atlas filename expression" ) );
  }

  const QString directory = parameterAsFileOutput( parameters, QStringLiteral( "FOLDER" ), context );
  QString fileName = QDir( directory ).filePath( QStringLiteral( "atlas" ) );

  QStringList imageFormats;
  const auto supportedImageFormats { QImageWriter::supportedImageFormats() };
  for ( const auto format : QImageWriter::supportedImageFormats() )
  {
    if ( format == QByteArray( "svg" ) )
      continue;
    imageFormats << QString( format );
  }
  int idx = parameterAsEnum( parameters, QStringLiteral( "EXTENSION" ), context );
  QString extension = '.' + imageFormats.at( idx );

  QgsLayoutExporter exporter( layout );
  QgsLayoutExporter::ImageExportSettings settings;

  if ( parameters.value( QStringLiteral( "DPI" ) ).isValid() )
  {
    settings.dpi = parameterAsDouble( parameters, QStringLiteral( "DPI" ), context );
  }

  settings.exportMetadata = parameterAsBool( parameters, QStringLiteral( "INCLUDE_METADATA" ), context );
  settings.generateWorldFile = parameterAsBool( parameters, QStringLiteral( "GEOREFERENCE" ), context );

  if ( parameterAsBool( parameters, QStringLiteral( "ANTIALIAS" ), context ) )
    settings.flags = settings.flags | QgsLayoutRenderContext::FlagAntialiasing;
  else
    settings.flags = settings.flags & ~QgsLayoutRenderContext::FlagAntialiasing;

  const QList< QgsMapLayer * > layers = parameterAsLayerList( parameters, QStringLiteral( "LAYERS" ), context );
  if ( layers.size() > 0 )
  {
    const QList<QGraphicsItem *> items = layout->items();
    for ( QGraphicsItem *graphicsItem : items )
    {
      QgsLayoutItem *item = dynamic_cast<QgsLayoutItem *>( graphicsItem );
      QgsLayoutItemMap *map = dynamic_cast<QgsLayoutItemMap *>( item );
      if ( map && !map->followVisibilityPreset() && !map->keepLayerSet() )
      {
        previousMapItemLayersReset << map->uuid();
        map->setLayers( layers );
      }
    }
  }

  QgsLayoutExporter::ExportResult result = exporter.exportToImage( atlas, fileName, extension, settings, error, feedback );
  restoreAtlas();

  switch ( result )
  {
    case QgsLayoutExporter::Success:
    {
      feedback->pushInfo( QObject::tr( "Successfully exported layout to %1" ).arg( QDir::toNativeSeparators( directory ) ) );
      break;
    }

    case QgsLayoutExporter::FileError:
      throw QgsProcessingException( QObject::tr( "Cannot write to %1.\n\nThis file may be open in another application." ).arg( QDir::toNativeSeparators( directory ) ) );

    case QgsLayoutExporter::MemoryError:
      throw QgsProcessingException( QObject::tr( "Trying to create the image "
                                    "resulted in a memory overflow.\n\n"
                                    "Please try a lower resolution or a smaller paper size." ) );

    case QgsLayoutExporter::IteratorError:
      throw QgsProcessingException( QObject::tr( "Error encountered while exporting atlas." ) );

    case QgsLayoutExporter::SvgLayerError:
    case QgsLayoutExporter::PrintError:
    case QgsLayoutExporter::Canceled:
      // no meaning for imageexports, will not be encountered
      break;
  }

  feedback->setProgress( 100 );

  QVariantMap outputs;
  outputs.insert( QStringLiteral( "FOLDER" ), directory );
  return outputs;
}

///@endcond

