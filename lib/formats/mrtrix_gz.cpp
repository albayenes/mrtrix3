/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 02/10/13.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "header.h"
#include "image_io/gz.h"
#include "formats/list.h"
#include "formats/mrtrix_utils.h"

namespace MR
{
    namespace Formats
    {


      std::unique_ptr<ImageIO::Base> MRtrix_GZ::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".mif.gz"))
          return std::unique_ptr<ImageIO::Base>();

        File::GZ zf (H.name(), "r");
        std::string first_line = zf.getline();
        if (first_line != "mrtrix image") {
          zf.close();
          throw Exception ("invalid first line for compressed image \"" + H.name() + "\" (expected \"mrtrix image\", read \"" + first_line + "\")");
        }
        read_mrtrix_header (H, zf);
        zf.close();

        std::string fname;
        size_t offset;
        get_mrtrix_file_path (H, "file", fname, offset);
        if (fname != H.name())
          throw Exception ("GZip-compressed MRtrix format images must have image data within the same file as the header");

        std::stringstream header;
        header << "mrtrix image\n";
        write_mrtrix_header (H, header);
        offset = header.str().size() + size_t(24);
        offset += ((4 - (offset % 4)) % 4);
        header << "file: . " << offset << "\nEND\n";
        
        std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, offset));
        memcpy (io_handler.get()->header(), header.str().c_str(), header.str().size());
        memset (io_handler.get()->header() + header.str().size(), 0, offset - header.str().size());
        io_handler->files.push_back (File::Entry (H.name(), offset));

        return std::move (io_handler);
      }





      bool MRtrix_GZ::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".mif.gz"))
          return false;

        H.set_ndim (num_axes);
        for (size_t i = 0; i < H.ndim(); i++)
          if (H.size (i) < 1)
            H.size(i) = 1;

        return true;
      }





      std::unique_ptr<ImageIO::Base> MRtrix_GZ::create (Header& H) const
      {
        std::stringstream header;
        header << "mrtrix image\n";
        write_mrtrix_header (H, header);

        int64_t offset = header.tellp() + int64_t(24);
        offset += ((4 - (offset % 4)) % 4);
        header << "file: . " << offset << "\nEND\n";
        while (header.tellp() < offset)
          header << '\0';

        std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, offset));
        memcpy (io_handler->header(), header.str().c_str(), offset);

        File::create (H.name());
        io_handler->files.push_back (File::Entry (H.name(), offset));

        return std::move (io_handler);
      }

    }
}
