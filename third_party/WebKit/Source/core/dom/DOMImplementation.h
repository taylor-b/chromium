/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef DOMImplementation_h
#define DOMImplementation_h

#include "core/CoreExport.h"
#include "core/dom/Document.h"

namespace blink {

class Document;
class DocumentInit;
class DocumentType;
class ExceptionState;
class HTMLDocument;
class XMLDocument;

class CORE_EXPORT DOMImplementation final
    : public GarbageCollected<DOMImplementation>,
      public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static DOMImplementation* Create(Document& document) {
    return new DOMImplementation(document);
  }

  // DOM methods & attributes for DOMImplementation
  bool hasFeature() { return true; }
  DocumentType* createDocumentType(const AtomicString& qualified_name,
                                   const String& public_id,
                                   const String& system_id,
                                   ExceptionState&);
  XMLDocument* createDocument(const AtomicString& namespace_uri,
                              const AtomicString& qualified_name,
                              DocumentType*,
                              ExceptionState&);

  // From the HTMLDOMImplementation interface
  HTMLDocument* createHTMLDocument(const String& title);

  // Other methods (not part of DOM)
  static Document* createDocument(const String& mime_type,
                                  const DocumentInit&,
                                  bool in_view_source_mode);

  static bool IsXMLMIMEType(const String&);
  static bool IsTextMIMEType(const String&);
  static bool IsJSONMIMEType(const String&);

  DECLARE_TRACE();

 private:
  explicit DOMImplementation(Document&);

  Member<Document> document_;
};

}  // namespace blink

#endif  // DOMImplementation_h
