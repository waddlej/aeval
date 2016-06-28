#ifndef __DSA_GRAPH_HH_
#define __DSA_GRAPH_HH_

#include <boost/core/noncopyable.hpp>
#include <boost/container/flat_map.hpp>

#include "llvm/ADT/ImmutableSet.h"

namespace llvm
{
  class Value;
  class Type;
  class DataLayout;
}

namespace seahorn
{
  namespace dsa
  {
    class Node;
    class Cell;
    class Graph
    {
      friend class Node;
    protected:
      
      typedef llvm::ImmutableSet<llvm::Type*> Set;
      typedef typename Set::Factory SetFactory;
      SetFactory m_setFactory;
      
      std::vector<std::unique_ptr<Node> > m_nodes;
      
      SetFactory &getSetFactory () { return m_setFactory; }
      Set emptySet () { return m_setFactory.getEmptySet (); }
      /// return a new set that is the union of old and a set containing v
      Set mkSet (Set old, const llvm::Type *v) { return m_setFactory.add (old, v); }

      const llvm::DataLayout &getDataLayout ();
    public:
      Node &mkNode ();
    };
    
    /** 
        A memory cell (or a field). An offset into a memory object.
     */
    class Cell
    {
      /// memory object
      mutable Node *m_node;
      /// offset
      mutable unsigned m_offset;

    public:
      Cell () : m_node(nullptr), m_offset (0) {}
      Cell (Node *node, unsigned offset) : m_node (node), m_offset (offset) {}
      Cell (const Cell &o) : m_node (o.m_node), m_offset (o.m_offset) {}
      Cell &operator= (const Cell &o)
      { m_node = o.m_node; m_offset = o.m_offset; return *this; }
      
      bool operator== (const Cell &o) const
      {return m_node == o.m_node && m_offset == o.m_offset;}
      bool operator< (const Cell &o) const
      { return m_node < o.m_node || (m_node == o.m_node && m_offset < o.m_offset); }
      
      bool isNull () const { return m_node == nullptr; }
      Node *getNode () const; 
      unsigned getOffset () const { return m_offset; }

      void pointTo (Node &n, unsigned offset);

      inline bool hasLink (unsigned offset = 0) const;
      inline const Cell &getLink (unsigned offset = 0) const;
      inline void setLink (unsigned offset, const Cell &c);
      inline void addLink (unsigned offset, Cell &c);
      
      /// unify with a given cell. At the end, both cells point to the
      /// same offset of the same node. Might cause collapse of the
      /// nodes represented by the cells.
      void unify (Cell &c);
      
      void swap (Cell &o)
      { std::swap (m_node, o.m_node); std::swap (m_offset, o.m_offset); }
    };
    
    
    /// A node of a DSA graph representing a memory object
    class Node : private boost::noncopyable
    {
      friend class Graph;
      friend class Cell;
      struct NodeType
      {
        unsigned shadow:1;
        unsigned alloca:1;
        unsigned heap:1;
        unsigned global:1;
        unsigned externFunc:1;
        unsigned externGlobal:1;
        unsigned unknown:1;
        unsigned incomplete:1;
        unsigned modified:1;
        unsigned read:1;
        unsigned array:1;
        unsigned collapsed:1;
        unsigned external:1;
        unsigned inttoptr:1;
        unsigned ptrtoint:1;
        unsigned vastart:1;
        unsigned dead:1;

        NodeType () {reset();}
        void join (const NodeType &n)
        {
          shadow |= n.shadow;
          alloca |= n.alloca;
          heap |= n.heap;
          global |= n.global;
          externFunc |= n.externFunc;
          externGlobal |= n.externGlobal;
          unknown |= n.unknown;
          incomplete |= n.incomplete;
          modified |= n.modified;
          read |= n.read;
          array |= n.array;
          collapsed |= n.collapsed;
          external |= n.external;
          inttoptr |= n.inttoptr;
          ptrtoint |= n.ptrtoint;
          vastart |= n.vastart;
          dead |= n.dead;
        }
        void reset () { memset (this, 0, sizeof (*this)); }
      };
      
      /// parent DSA graph
      Graph *m_graph;
      /// node marks
      struct NodeType m_nodeType;
      /// TODO: UNUSED
      mutable const llvm::Value *m_unique_scalar;
      /// When the node is forwarding, the memory cell at which the
      /// node begins in some other memory object
      Cell m_forward;

      typedef Graph::Set Set;
      typedef boost::container::flat_map<unsigned,  Set> types_type;
      typedef boost::container::flat_map<unsigned, Cell> links_type;
      /// known type of every offset/field
      types_type m_types;
      /// destination of every offset/field
      links_type m_links;
      
      /// known size
      unsigned m_size;

      Node (Graph &g) : m_graph (&g), m_unique_scalar (nullptr), m_size (0) {}
      
      /// adjust offset based on type of the node Collapsed nodes
      /// always have offset 0; for array nodes the offset is modulo
      /// array size; otherwise offset is not adjusted
      unsigned adjustOffset (unsigned offset) const;
      
      /// Unify a given node with a specified offset of the current node
      /// post-condition: the given node points to the current node.
      /// might cause a collapse
      void unifyAt (Node &n, unsigned offset);
      
      /// Transfer links/types and other information from the current
      /// node to the given one at a given offset and make the current
      /// one point to the result. Might cause collapse.  Most clients
      /// should use unifyAt() that has less stringent preconditions.
      void pointTo (Node &node, unsigned offset);
      
      Cell &getLink (unsigned offset) {return m_links [adjustOffset (offset)];}
      
      /// Adds a set of types for a field at a given offset
      void addType (unsigned offset, Set types);
      
      /// joins all the types of a given node starting at a given
      /// offset of the current node
      void joinTypes (unsigned offset, const Node &n);
    public:
      /// unify with a given node
      void unify (Node &n) { unifyAt (n, 0); }
      Node &setAlloca (bool v = true) { m_nodeType.alloca = v; return *this;}
      bool isAlloca () const { return m_nodeType.alloca; }

      Node &setArray (bool v = true) { m_nodeType.array = v; return *this; }
      bool isArray () const { return m_nodeType.array; }

      Node &setCollapsed (bool v = true) { m_nodeType.collapsed = v; return *this; }
      bool isCollapsed () const { return m_nodeType.collapsed; }
      
      bool isUnique () const { return m_unique_scalar; }
      
      inline bool isForwarding () const;
      
      
      /// Return a node the current node represents. If the node is
      /// forwarding, returns the non-forwarding node this node points
      /// to. Might be expensive.
      inline Node* getNode ();
      inline const Node* getNode () const;
      unsigned getOffset () const;

      const types_type &types () { return m_types; }
      const links_type &links () { return m_links; }

      unsigned size () { return m_size; }
      void growSize (unsigned v) {if (v > m_size) m_size = v;}
      
      /// increase size to accommodate a field of type t at the given offset
      void growSize (unsigned offset, const llvm::Type *t);

      bool hasLink (unsigned offset) const
      { return m_links.count (adjustOffset (offset)) > 0; }
      const Cell &getLink (unsigned offset) const
      {return m_links.at (adjustOffset (offset));}
      void setLink (unsigned offset, const Cell &c) {getLink (offset) = c;}
      void addLink (unsigned offset, Cell &c);

      bool hasType (unsigned offset) const;
      
      const Set getType (unsigned offset) const
      { return m_types.at (adjustOffset (offset));}
      bool isVoid () const { return m_types.empty (); }
      bool isEmtpyType () const;
      
      /// Adds a type of a field at a given offset
      void addType (unsigned offset, const llvm::Type *t);
      
      /// collapse the current node. Looses all field sensitivity
      void collapse ();
      
    };

    bool Node::isForwarding () const
    { return !m_forward.isNull (); }

    bool Cell::hasLink (unsigned offset) const
    {return m_node && getNode ()->hasLink (m_offset + offset);}

    const Cell &Cell::getLink (unsigned offset) const
    {
      assert (m_node);
      return getNode ()->getLink(offset + m_offset);
    }

    void Cell::setLink (unsigned offset, const Cell &c)
    { getNode ()->setLink(m_offset + offset, c); }

    void Cell::addLink (unsigned offset, Cell &c)
    { getNode ()->addLink (m_offset + offset, c); }
    
    
    Node* Node::getNode () 
    { return isForwarding () ? m_forward.getNode () : this;}

    const Node* Node::getNode () const
    { return isForwarding () ? m_forward.getNode () : this;}
  }
}

#endif