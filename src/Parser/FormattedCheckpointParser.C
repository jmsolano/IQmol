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

#include "FormattedCheckpointParser.h"
#include "MolecularOrbitalsList.h"
#include "DipoleMoment.h"
#include "GeometryList.h"
#include "TextStream.h"
#include "Constants.h"
#include "Hessian.h"
#include "Energy.h"
#include "QsLog.h"
#include <QtDebug>
#include <cmath>


namespace IQmol {
namespace Parser {

bool FormattedCheckpoint::parse(TextStream& textStream)
{
   Data::GeometryList* geometryList(new Data::GeometryList);
   Data::MolecularOrbitalsList* molecularOrbitalsList(new Data::MolecularOrbitalsList);
   Data::Geometry* geometry(0);

   bool ok(true);
   GeomData  geomData;
   ShellData shellData;
   MoData    moData;

   while (!textStream.atEnd()) {

      QString line(textStream.nextLine());
      QString key(line);
      key.resize(42);
      key = key.trimmed();
      QString tmp(line.mid(43, 37));

      QStringList list(TextStream::tokenize(tmp));

      if (key == "Number of alpha electrons") {            // This should only appear once
         moData.nAlpha = list.at(1).toInt(&ok);
         if (!ok) goto error;

      }else if (key == "Number of beta electrons") {       // This should only appear once
         moData.nBeta = list.at(1).toInt(&ok);
         if (!ok) goto error;

      }else if (key == "Atomic numbers") {                 // This should only appear once
         unsigned n(list.at(2).toUInt(&ok));  
         if (!ok) goto error;
         geomData.atomicNumbers = readUnsignedArray(textStream, n);

      }else if (key == "Current cartesian coordinates") { // This triggers a new geometry

         if (geometry) {
            // We are on a subsequent geometry, so we should have everything we need to
            // create the MOs for the previous one.
            Data::MolecularOrbitals* mos(makeMolecularOrbitals(moData, shellData, *geometry)); 
            clear(moData);
            if (mos) molecularOrbitalsList->append(mos);
         }

         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         geomData.coordinates = readDoubleArray(textStream, n);
         geometry = makeGeometry(geomData);
         if (!geometry) goto error;
         geometryList->append(geometry);

      }else if (key == "Number of basis functions") {
         // Determined from the size of the MO coefficient matrices

      }else if (key == "Shell types") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         shellData.shellTypes = readIntegerArray(textStream, n);
         
      }else if (key == "Number of primitives per shell") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         shellData.shellPrimitives = readUnsignedArray(textStream, n);

      }else if (key == "Shell to atom map") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         shellData.shellToAtom = readUnsignedArray(textStream, n);

      }else if (key == "Primitive exponents") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         shellData.exponents = readDoubleArray(textStream, n);

      }else if (key == "Contraction coefficients") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         shellData.contractionCoefficients = readDoubleArray(textStream, n);

      }else if (key == "P(S=P) Contraction coefficients") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         shellData.contractionCoefficientsSP = readDoubleArray(textStream, n);

      }else if (key == "SCF Energy") {
         double energy(list.at(1).toDouble(&ok));
         if (!ok || !geometry) goto error;
         Data::ScfEnergy& scf(geometry->getProperty<Data::ScfEnergy>());
         scf.setValue(energy, Data::Energy::Hartree);
         Data::TotalEnergy& total(geometry->getProperty<Data::TotalEnergy>());
         total.setValue(energy, Data::Energy::Hartree);

      }else if (key == "Total Energy") {
         double energy(list.at(1).toDouble(&ok));
         if (!ok || !geometry) goto error;
         Data::TotalEnergy& total(geometry->getProperty<Data::TotalEnergy>());
         total.setValue(energy, Data::Energy::Hartree);

      }else if (key == "Alpha MO coefficients") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         moData.alphaCoefficients = readDoubleArray(textStream, n);
         moData.betaCoefficients  = moData.alphaCoefficients;

      }else if (key == "Beta MO coefficients") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         moData.betaCoefficients = readDoubleArray(textStream, n);

      }else if (key == "Alpha Orbital Energies") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok) goto error;
         moData.alphaEnergies = readDoubleArray(textStream, n);
         moData.betaEnergies  = moData.alphaEnergies;

      }else if (key == "Beta Orbital Energies") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok || !geometry) goto error;
         moData.betaEnergies = readDoubleArray(textStream, n);

      }else if (key == "Dipole_Data") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok || !geometry) goto error;
         QList<double> data(readDoubleArray(textStream, n));
         if (data.size() != 3) goto error;
         Data::DipoleMoment& dipole(geometry->getProperty<Data::DipoleMoment>());
         dipole.setValue(data[0],data[1],data[2]);

      }else if (key == "Cartesian Force Constants") {
         unsigned n(list.at(2).toUInt(&ok));
         if (!ok || !geometry) goto error;
         QList<double> data(readDoubleArray(textStream, n));
         Data::Hessian& hessian(geometry->getProperty<Data::Hessian>());
         hessian.setData(geometry->nAtoms(), data);
      }

   }

   if (geometry) {
      Data::MolecularOrbitals* mos(makeMolecularOrbitals(moData, shellData, *geometry)); 
      if (mos) molecularOrbitalsList->append(mos);
   }

   if (geometryList) {
      if (geometryList->isEmpty()) {
         delete geometryList;
      }else {
         geometryList->setDefaultIndex(-1);
         m_dataBank.append(geometryList);
      }
   }

   if (molecularOrbitalsList) {
      if (molecularOrbitalsList->isEmpty()) {
         delete molecularOrbitalsList;
      }else {
         molecularOrbitalsList->setDefaultIndex(-1);
         m_dataBank.append(molecularOrbitalsList);
      }
   }
   
   return m_errors.isEmpty();
 
   error:
      QString msg("Error parsing checkpoint data around line number ");
      msg += QString::number(textStream.lineNumber());
      m_errors.append(msg);

   delete geometryList;
   delete molecularOrbitalsList;

   return false;
}


void FormattedCheckpoint::clear(MoData& moData)
{
   moData.alphaCoefficients.clear();
   moData.betaCoefficients.clear();
   moData.alphaEnergies.clear();
   moData.betaEnergies.clear();
}


Data::Geometry* FormattedCheckpoint::makeGeometry(GeomData const& geomData)
{
   unsigned nAtoms(geomData.atomicNumbers.size());
   Data::Geometry* geom(0);
   if (nAtoms > 0 && 3*nAtoms == (unsigned)geomData.coordinates.size()) {
      geom = new Data::Geometry(geomData.atomicNumbers, geomData.coordinates);
      geom->scaleCoordinates(Constants::BohrToAngstrom);
      geom->computeGasteigerCharges();
   }
   return geom;
}


bool FormattedCheckpoint::dataAreConsistent(ShellData const& shellData, unsigned const nAtoms)
{
   for (int i = 0; i < shellData.shellToAtom.size(); ++i) {
       int atomIndex(shellData.shellToAtom[i]);
       if (atomIndex < 1 || atomIndex > (int)nAtoms) {
          QString msg("Invalid atom index ");
          msg += QString::number(atomIndex) + " ";
          msg += QString::number(nAtoms);
          m_errors.append(msg);
          return false;
       }
   }

   int nPrimitives(0);
   int nShells(shellData.shellTypes.size());
   for (int i = 0; i < shellData.shellPrimitives.size(); ++i) {
       nPrimitives += shellData.shellPrimitives[i];
   }

   if ( shellData.shellToAtom.size()               != nShells     ||
        shellData.shellPrimitives.size()           != nShells     ||
        shellData.exponents.size()                 != nPrimitives ||
        shellData.contractionCoefficients.size()   != nPrimitives ||
       (shellData.contractionCoefficientsSP.size() != nPrimitives &&
        shellData.contractionCoefficientsSP.size() != 0)      ) {
       QLOG_WARN() << "Inconsistent checkpoint data:";
       QLOG_WARN() << "  Shells" << shellData.shellToAtom.size()               << nShells;
       QLOG_WARN() << "  Prims " << shellData.shellPrimitives.size()           << nShells;
       QLOG_WARN() << "  Expts " << shellData.exponents.size()                 << nPrimitives;
       QLOG_WARN() << "  CCs   " << shellData.contractionCoefficients.size()   << nPrimitives;
       QLOG_WARN() << "  SP CCs" << shellData.contractionCoefficientsSP.size() << nPrimitives;
       m_errors.append("Inconsistent shell data read from file");
       return false;
   }
   return true;
}



Data::MolecularOrbitals* FormattedCheckpoint::makeMolecularOrbitals(MoData const& moData, 
   ShellData const& shellData, Data::Geometry const& geometry)
{
   // This needs fixing.  Newer versions of QChem only print the orbitals for the
   // final geometry, so the first ones are just for the geometries
   if (moData.alphaEnergies.isEmpty()) return 0;
   Data::ShellList* shellList = makeShellList(shellData, geometry);
   if (!shellList) return 0;
   Data::MolecularOrbitals* mos = new Data::MolecularOrbitals(
      moData.nAlpha, 
      moData.nBeta,
      moData.alphaCoefficients,
      moData.alphaEnergies,
      moData.betaCoefficients,
      moData.betaEnergies,
      *shellList
   );

   if (!mos->consistent()) {
      QString msg("Data are inconsistent. Check shell types.");
      m_errors.append(msg);
      delete mos;
      mos = 0;
   }

   return mos;
}
            


Data::ShellList* FormattedCheckpoint::makeShellList(ShellData const& shellData, 
   Data::Geometry const& geometry)
{
   if (!dataAreConsistent(shellData, geometry.nAtoms())) return 0;

   static double const convExponents(std::pow(Constants::BohrToAngstrom, -2.0));
   Data::ShellList* shellList(new Data::ShellList);
   unsigned nShells(shellData.shellTypes.size());
   unsigned cnt(0);

   for (unsigned shell = 0; shell < nShells; ++shell) {

       QList<double> expts;
       QList<double> coefs;
       QList<double> coefsSP;

       unsigned atom(shellData.shellToAtom.at(shell)-1);
       qglviewer::Vec position(geometry.position(atom));

       for (unsigned i = 0; i < shellData.shellPrimitives.at(shell); ++i, ++cnt) {
		   // Convert exponents from bohr to angstrom.  The conversion factor
		   // for the coefficients depends on the angular momentum and the 
           // conversion is effectively done Shell constructor
           expts.append(shellData.exponents.at(cnt)*convExponents);

           coefs.append(shellData.contractionCoefficients.at(cnt));
           if (!shellData.contractionCoefficientsSP.isEmpty()) {
              coefsSP.append(shellData.contractionCoefficientsSP.at(cnt));
           }
       }

       switch (shellData.shellTypes.at(shell)) {
          case 0:
             shellList->append( new Data::Shell(Data::Shell::S, position, expts, coefs) );
             break;
          case -1:
             shellList->append( new Data::Shell(Data::Shell::S, position, expts, coefs)   );
             shellList->append( new Data::Shell(Data::Shell::P, position, expts, coefsSP) );
             break;
          case 1:
             shellList->append( new Data::Shell(Data::Shell::P, position, expts, coefs) );
             break;
          case -2:
             shellList->append( new Data::Shell(Data::Shell::D5, position, expts, coefs) );
             break;
          case 2:
             shellList->append( new Data::Shell(Data::Shell::D6, position, expts, coefs) );
             break;
          case -3:
             shellList->append( new Data::Shell(Data::Shell::F7, position, expts, coefs) );
             break;
          case 3:
             shellList->append( new Data::Shell(Data::Shell::F10, position, expts, coefs) );
             break;
          case -4:
             shellList->append( new Data::Shell(Data::Shell::G9, position, expts, coefs) );
             break;
          case 4:
             shellList->append( new Data::Shell(Data::Shell::G15, position, expts, coefs) );
             break;

          default:
             delete shellList;
             QString msg("Unknown Shell type found at position ");
             msg += QString::number(shell);
             msg += ", type: "+ QString::number(shellData.shellTypes.at(shell));
             m_errors.append(msg);
             return 0;
             break;

       }
   }

   return shellList;
}


QList<int> FormattedCheckpoint::readIntegerArray(TextStream& textStream, unsigned n)
{
   bool ok;
   QList<int> values;

   while ((unsigned)values.size() < n) {
      QStringList tokens(textStream.nextLineAsTokens());
      for (unsigned i = 0; i < (unsigned)tokens.size(); ++i) {
          int v(tokens.at(i).toInt(&ok));
          if (!ok) goto error;
          values.append(v);
      }
   }

   if ((unsigned)values.size() == n) return values;

   error:
      QString msg("Error parsing checkpoint data around line number ");
      msg += QString::number(textStream.lineNumber()) + "\n";
      msg += "Expected integer value";
      m_errors.append(msg);

   return QList<int>();
}


QList<unsigned> FormattedCheckpoint::readUnsignedArray(TextStream& textStream, unsigned n)
{
   bool ok;
   QList<unsigned> values;

   while ((unsigned)values.size() < n) {
      QStringList tokens(textStream.nextLineAsTokens());
      for (unsigned i = 0; i < (unsigned)tokens.size(); ++i) {
          unsigned v(tokens.at(i).toUInt(&ok));
          if (!ok) goto error;
          values.append(v);
      }
   }

   if ((unsigned)values.size() == n) return values;

   error:
      QString msg("Error parsing checkpoint data around line number ");
      msg += QString::number(textStream.lineNumber()) + "\n";
      msg += "Expected unsigned integer value";
      m_errors.append(msg);

   return QList<unsigned>();
}


QList<double> FormattedCheckpoint::readDoubleArray(TextStream& textStream, unsigned n)
{
   bool ok;
   QList<double> values;

   while ((unsigned)values.size() < n) {
      QString line(textStream.readLine());
      while (!line.isEmpty()) {
         QString token(line.left(16));
         double v(token.toDouble(&ok));
         if (!ok) goto error;
         values.append(v);
         line.remove(0,token.size());
      }
   }

   if ((unsigned)values.size() == n) return values;

   error:
      QString msg("Error parsing checkpoint data around line number ");
      msg += QString::number(textStream.lineNumber()) + "\n";
      msg += "Expected double value";
      m_errors.append(msg);

   return QList<double>();
}

} } // end namespace IQmol::Parser
