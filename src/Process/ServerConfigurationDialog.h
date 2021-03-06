#ifndef IQMOL_PROCESS_SERVERCONFIGURATIONDIALOG_H
#define IQMOL_PROCESS_SERVERCONFIGURATIONDIALOG_H
/*******************************************************************************
         
  Copyright (C) 2011-2013 Andrew Gilbert
      
  This file is part of IQmol, a free molecular visualization program. See
  <http://iqmol.org> for more details.
         
  IQmol is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software  
  Foundation, either version 3 of the License, or (at your option) any later  
  version.

  IQmol is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.
      
  You should have received a copy of the GNU General Public License along
  with IQmol.  If not, see <http://www.gnu.org/licenses/>.
   
********************************************************************************/

#include "ui_ServerConfigurationDialog.h"


namespace IQmol {
namespace Process2 {

   class ServerConfiguration;

   class ServerConfigurationDialog : public QDialog {

      Q_OBJECT

      public:
         ServerConfigurationDialog(ServerConfiguration& config, QWidget* parent = 0);
         ~ServerConfigurationDialog();

      private Q_SLOTS:
         void on_localRadioButton_toggled(bool);
         void on_sshRadioButton_toggled(bool);
         void on_httpRadioButton_toggled(bool);
         void on_httpsRadioButton_toggled(bool);

         void on_configureConnectionButton_clicked(bool);
         void on_configureQueueButton_clicked(bool);
         void on_queueSystem_currentIndexChanged(QString const& queue);

         void on_testConnectionButton_clicked(bool);
         void on_loadButton_clicked(bool);
         void on_exportButton_clicked(bool);

         void verify();

      private:
         bool testConnection();
         bool testSshConnection(ServerConfiguration const&);
         bool testHttpConnection(ServerConfiguration const&);

         void copyFrom(ServerConfiguration const&);
         bool copyTo(ServerConfiguration*);

         void init();
         void updateAllowedQueueSystems(bool httpOnly);

         bool m_tested;

         bool blockUpdate() const;
         void blockUpdate(bool const tf);
         bool m_blockUpdate;

         Ui::ServerConfigurationDialog m_dialog;

         ServerConfiguration& m_originalConfiguration;
         ServerConfiguration* m_currentConfiguration;
   };

} } // end namespace IQmol::Process

#endif
