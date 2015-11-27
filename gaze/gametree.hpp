#ifndef GAMETREE_HPP
#define GAMETREE_HPP

#include<boost/graph/adjacency_list.hpp>
#include<boost/graph/graph_traits.hpp>

#define DEBUG 0
#include "../logging.hpp"

namespace gaze {

template<typename game_state>
class game_tree;

template<typename game_tree>
class viterator : public std::iterator<std::forward_iterator_tag, typename game_tree::vertex_property> {
  typedef typename game_tree::vertex_property vertex;
  typedef typename vertex::graph_edge_it graph_edge_it;
  typedef typename game_tree::graph graph;
  graph_edge_it it;
  graph& g;
public:
  viterator(graph_edge_it it, graph& g): it(it), g(g){}
  viterator& operator=(const viterator& rhs) { return *this; }
  viterator& operator++() {++it; return *this;};
  viterator operator++(int) {viterator res(*this); ++(*this); return res;}
  bool operator==(const viterator& rhs) { return it==rhs.it; }
  bool operator!=(const viterator& rhs) { return it!=rhs.it; }
  vertex& operator*() {
    return g[boost::target(*it, g)];
  }
};

template<typename game_tree>
class vertex {
public:
  typedef typename game_tree::graph graph;
  typedef typename game_tree::state state;
  typedef vertex<game_tree> vertex_property;
  typedef typename boost::graph_traits<graph>::out_edge_iterator graph_edge_it;
  typedef typename gaze::viterator<game_tree> vertex_iterator;

  typedef typename boost::graph_traits<graph>::vertex_descriptor vertex_descriptor;

  vertex() {}

  vertex(const vertex& old):st(old.st),vd(old.vd),gt(old.gt),g(old.g),level(old.level) {}

  vertex(state* st, vertex_descriptor vd, int level, game_tree* gt): st(st), vd(vd), gt(gt), g(&gt->g), level(level) {}

  vertex& operator=(const vertex& other){
    if(&other != this) {
      this->st = other.st;
      this->gt = other.gt;
      this->g = other.g;
      this->vd = other.vd;
      this->level = other.level;
      this->children_added = other.children_added;
      return *this;
    }
  }

  state& get_state() { return *st; }

  std::pair<vertex_iterator, vertex_iterator> get_children() {
    dout<<"hh "<<get_state()<<" "<<children_added<<std::endl;
    if(!(*g)[vd].children_added) {
      add_children();
    }
    typename boost::graph_traits<graph>::out_edge_iterator begin_edge_it, end_edge_it;
    boost::tie(begin_edge_it, end_edge_it) = boost::out_edges(vd, *g);

    return std::make_pair(vertex_iterator(begin_edge_it, *g), vertex_iterator(end_edge_it, *g));
  }

  bool operator==(const vertex& vt) {
    return vd==vt.vd;
  }
  size_t get_children_count() {
    if(!(*g)[vd].children_added) {
      add_children();
    }
    return boost::out_degree(vd, *g);
  }

  std::ostream& print(std::ostream& os) {
    os<<"("<<get_state();
    if((*g)[vd].children_added) {
      os<<" degree "<<get_children_count()<<": ";
      auto it_pair = get_children();
      for_each(it_pair.first, it_pair.second, [&](auto vert){ os<<vert; });
    }
    os<<")";
    return os;
  }

  vertex_descriptor get_vd() {
    return vd; 
  }

 template<typename U>
 friend std::ostream& operator<<(std::ostream&, vertex<U>& );

private:
  void add_children() {
    auto container = st->get_children();
    for(auto it=container.begin(); it!=container.end(); it++){
      auto tvd = boost::add_vertex(*g);
      vertex_property nvert(*it,tvd,level+1,gt);
      (*g)[tvd] = nvert;
      boost::add_edge(vd, tvd, *g);
    }
    (*g)[vd].children_added = true;
  }

  int level=0;
  vertex_descriptor vd;
  state* st = nullptr;
  game_tree* gt = nullptr;
  graph* g = nullptr;
  bool children_added = false;
};

template<typename game_state>
class game_tree {
public:
  typedef game_state state;
  typedef vertex<game_tree> vertex_property;
  typedef boost::adjacency_list<boost::listS, boost::listS, boost::directedS,
                        vertex_property> graph;
  typedef typename boost::graph_traits<graph>::vertex_descriptor vertex_descriptor;

  typedef typename vertex_property::vertex_iterator vertex_iterator;

  game_tree(state* st) {
    auto vd = boost::add_vertex(g);
    vertex_property root(st,vd,0,this);
    g[vd] = root;
    root_vertex = cur_vertex = &(g[vd]);
  }
  game_tree(const game_tree& otherTree);

  //to be called for all moves
  void set_current_state(state committedstate) {
  }
  //returns the previously committed vetex, used by algo
  vertex_property& get_current_vertex() {return *cur_vertex;}
  //returns root vertex of the tree
  vertex_property& get_root_vertex() {return *root_vertex;}
  //returns the previously committed state, used by game designer
  state& get_current_state() {
    return cur_vertex->get_state();
  }
  graph g;

  std::ostream& print(std::ostream& os) {
    return os<<(*root_vertex);
  }
private:
  vertex_property *cur_vertex;
  vertex_property *root_vertex;
};

template<typename game_state>
std::ostream& operator<<(std::ostream& os, game_tree<game_state>& t)
{
#define PRINT 1
#if PRINT
  return t.print(os);
#else
  typedef typename game_tree<game_state>::graph graph;

  typedef typename boost::graph_traits<graph> GraphTraits;
  typename GraphTraits::out_edge_iterator out_i, out_end;
  typedef typename boost::graph_traits<graph>::vertex_descriptor Vertex;
  typedef typename boost::graph_traits<graph>::vertex_iterator vertex_iter;
  typename GraphTraits::edge_descriptor e;

  std::pair<vertex_iter, vertex_iter> vp;
  graph& g = t.g;
  vp = boost::vertices(g);
  std::for_each(vp.first,vp.second,[&](auto p){
    os<< (p) << "->";
    if(boost::out_degree(p, g)) {
      for (boost::tie(out_i, out_end) = boost::out_edges(p, g);
         out_i != out_end-1; ++out_i) {
        e = *out_i;
        Vertex src = boost::source(e, g), targ = boost::target(e, g);
        os<< targ << ",";
      }
      os<<target(*out_i,g);
    }
    os<<std::endl;
  });
  os<< std::endl;
  return os;
#endif
}

template<typename game_tree>
std::ostream& operator<<(std::ostream& os, vertex<game_tree>& t)
{
  //dout<<"vertex_descriptor "<<t.vd<<std::endl;
  //dout<<"children_added flag "<<(*(t.g))[t.vd].children_added<<std::endl;
  return t.print(os);
}

}

#endif