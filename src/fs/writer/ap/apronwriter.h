/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef WRITER_APRONWRITER_H_
#define WRITER_APRONWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/bgl/ap/apron.h"
#include "fs/bgl/ap/apron2.h"

namespace atools {
namespace fs {
namespace writer {

class ApronWriter :
  public atools::fs::writer::WriterBase<QPair<const atools::fs::bgl::Apron *, const atools::fs::bgl::Apron2 *> >
{
public:
  ApronWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "apron")
  {
  }

  virtual ~ApronWriter()
  {
  }

protected:
  virtual void writeObject(const QPair<const bgl::Apron *, const bgl::Apron2 *> *type) override;

private:
  QString toString(const QList<int>& triangles);
  QString toWkt(const QList<atools::fs::bgl::BglPosition>& vertices);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_APRONWRITER_H_ */