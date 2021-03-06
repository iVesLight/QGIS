/***************************************************************************
    qgssymbollevelsdialog.cpp
    ---------------------
    begin                : November 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssymbollevelsdialog.h"

#include "qgssymbollayerutils.h"
#include "qgssymbollayer.h"
#include "qgssymbol.h"
#include "qgssettings.h"

#include <QTableWidgetItem>
#include <QItemDelegate>
#include <QSpinBox>



////////////////

QgsSymbolLevelsDialog::QgsSymbolLevelsDialog( const QgsLegendSymbolList &list, bool usingSymbolLevels, QWidget *parent )
  : QDialog( parent )
  , mList( list )
  , mForceOrderingEnabled( false )
{
  setupUi( this );

  QgsSettings settings;
  restoreGeometry( settings.value( QStringLiteral( "Windows/symbolLevelsDlg/geometry" ) ).toByteArray() );

  tableLevels->setItemDelegate( new SpinBoxDelegate( this ) );

  chkEnable->setChecked( usingSymbolLevels );

  connect( chkEnable, &QAbstractButton::clicked, this, &QgsSymbolLevelsDialog::updateUi );
  connect( buttonBox, &QDialogButtonBox::helpRequested, this, &QgsSymbolLevelsDialog::showHelp );

  // only consider entries with symbols
  Q_FOREACH ( const QgsLegendSymbolItem &item, list )
  {
    if ( item.symbol() )
      mList << item;
  }

  int maxLayers = 0;
  tableLevels->setRowCount( mList.count() );
  for ( int i = 0; i < mList.count(); i++ )
  {
    QgsSymbol *sym = mList.at( i ).symbol();

    // set icons for the rows
    QIcon icon = QgsSymbolLayerUtils::symbolPreviewIcon( sym, QSize( 16, 16 ) );
    tableLevels->setVerticalHeaderItem( i, new QTableWidgetItem( icon, QString() ) );

    // find out max. number of layers per symbol
    int layers = sym->symbolLayerCount();
    if ( layers > maxLayers )
      maxLayers = layers;
  }

  tableLevels->setColumnCount( maxLayers + 1 );
  tableLevels->setHorizontalHeaderItem( 0, new QTableWidgetItem( QString() ) );
  for ( int i = 0; i < maxLayers; i++ )
  {
    QString name = tr( "Layer %1" ).arg( i );
    tableLevels->setHorizontalHeaderItem( i + 1, new QTableWidgetItem( name ) );
  }

  mMaxLayers = maxLayers;

  updateUi();

  if ( !usingSymbolLevels )
    setDefaultLevels();

  populateTable();

  connect( tableLevels, &QTableWidget::cellChanged, this, &QgsSymbolLevelsDialog::renderingPassChanged );
}

QgsSymbolLevelsDialog::~QgsSymbolLevelsDialog()
{
  QgsSettings settings;
  settings.setValue( QStringLiteral( "Windows/symbolLevelsDlg/geometry" ), saveGeometry() );
}

void QgsSymbolLevelsDialog::populateTable()
{
  for ( int row = 0; row < mList.count(); row++ )
  {
    QgsSymbol *sym = mList.at( row ).symbol();
    QString label = mList.at( row ).label();
    QTableWidgetItem *itemLabel = new QTableWidgetItem( label );
    itemLabel->setFlags( itemLabel->flags() ^ Qt::ItemIsEditable );
    tableLevels->setItem( row, 0, itemLabel );
    for ( int layer = 0; layer < mMaxLayers; layer++ )
    {
      QTableWidgetItem *item = nullptr;
      if ( layer >= sym->symbolLayerCount() )
      {
        item = new QTableWidgetItem();
        item->setFlags( Qt::ItemFlags() );
      }
      else
      {
        QgsSymbolLayer *sl = sym->symbolLayer( layer );
        QIcon icon = QgsSymbolLayerUtils::symbolLayerPreviewIcon( sl, QgsUnitTypes::RenderMillimeters, QSize( 16, 16 ) );
        item = new QTableWidgetItem( icon, QString::number( sl->renderingPass() ) );
      }
      tableLevels->setItem( row, layer + 1, item );
      tableLevels->resizeColumnToContents( 0 );
    }
  }

}

void QgsSymbolLevelsDialog::updateUi()
{
  tableLevels->setEnabled( chkEnable->isChecked() );
}

void QgsSymbolLevelsDialog::setDefaultLevels()
{
  for ( int i = 0; i < mList.count(); i++ )
  {
    QgsSymbol *sym = mList.at( i ).symbol();
    for ( int layer = 0; layer < sym->symbolLayerCount(); layer++ )
    {
      sym->symbolLayer( layer )->setRenderingPass( layer );
    }
  }
}

bool QgsSymbolLevelsDialog::usingLevels() const
{
  return chkEnable->isChecked();
}

void QgsSymbolLevelsDialog::renderingPassChanged( int row, int column )
{
  if ( row < 0 || row >= mList.count() )
    return;
  QgsSymbol *sym = mList.at( row ).symbol();
  if ( column < 0 || column > sym->symbolLayerCount() )
    return;
  sym->symbolLayer( column - 1 )->setRenderingPass( tableLevels->item( row, column )->text().toInt() );
}

void QgsSymbolLevelsDialog::setForceOrderingEnabled( bool enabled )
{
  mForceOrderingEnabled = enabled;
  if ( enabled )
  {
    chkEnable->setChecked( true );
    chkEnable->hide();
  }
  else
    chkEnable->show();
}

void QgsSymbolLevelsDialog::showHelp()
{
  QgsHelp::openHelp( QStringLiteral( "working_with_vector/vector_properties.html#symbols-levels" ) );
}


/// @cond PRIVATE

QWidget *SpinBoxDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem &, const QModelIndex & ) const
{
  QSpinBox *editor = new QSpinBox( parent );
  editor->setMinimum( 0 );
  editor->setMaximum( 999 );
  return editor;
}

void SpinBoxDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
  int value = index.model()->data( index, Qt::EditRole ).toInt();
  QSpinBox *spinBox = static_cast<QSpinBox *>( editor );
  spinBox->setValue( value );
}

void SpinBoxDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  QSpinBox *spinBox = static_cast<QSpinBox *>( editor );
  spinBox->interpretText();
  int value = spinBox->value();

  model->setData( index, value, Qt::EditRole );
}

void SpinBoxDelegate::updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & ) const
{
  editor->setGeometry( option.rect );
}


///@endcond
