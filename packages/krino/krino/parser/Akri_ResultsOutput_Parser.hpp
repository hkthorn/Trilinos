// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef Akri_ResultsOutput_Parser_h
#define Akri_ResultsOutput_Parser_h

namespace krino { class Region; }
namespace krino { namespace Parser { class Node; } }

namespace krino {
namespace ResultsOutput_Parser {
  void parse(const Parser::Node & node, Region & region);
}
}

#endif // Akri_ResultsOutput_Parser_h
