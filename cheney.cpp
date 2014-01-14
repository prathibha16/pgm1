// Pseudo-code that implements Cheney's algorithm

class Object {
  // remains null for normal objects
  // non-null for forwarded objects
  Object* _forwardee;

public:
  void forward_to(address new_addr);
  Object* forwardee();
  bool is_forwarded();
  size_t size();
  Iterator<Object**> object_fields();
};

class Heap {
  Semispace* _from_space;
  Semispace* _to_space;

  void swap_spaces();
  Object* evacuate(Object* obj);

public:
  address allocate(size_t size);
  void collect();
};

class Semispace {
  address _bottom;
  address _top;
  address _end;

public:
  address bottom() { return _bottom; }
  address top()    { return _top; }
  address end()    { return _end; }

  bool contains(address obj);
  address allocate(size_t size);
};

void Object::forward_to(address new_addr) {
  _forwardee = new_addr;
}

Object* forwardee() {
  return _forwardee;
}

bool Object::is_forwarded() {
  return _forwardee != nullptr;
}

void Heap::swap_spaces() {
  // std:swap(_from_space, _to_space);
  Semispace* temp = _from_space;
  _from_space = _to_space;
  _to_space = temp;
}

address Heap::allocate(size_t size) {
  return _from_space->allocate();
}

Object* Heap::evacuate(Object* obj) {
  size_t size = obj->size();

  // allocate space in to_space and copy object to there
  address new_addr = _to_space->allocate(size);
  copy(/* to */ new_addr, /* from */ obj, size);

  // set forwarding pointer in old object
  Object* new_obj = (Object*) new_addr;
  obj->forward_to(new_obj);

  return new_obj;
}

void Heap::collect() {
  address scanned = _to_space->bottom();
  
  // scavenge objects directly referenced by the root set
  foreach (Object** field in ROOTS) {
    Object* obj = *field;
    if (obj != nullptr && _from_space->contains(obj)) {
      Object* new_obj = obj->is_forwarded() ? obj->forwardee()
                                            : evacuate(obj);
      *field = new_obj;
    }
  }

  // breadth-first scanning of object graph
  do {
    Object* parent_obj = (Object*) scanned;
    foreach (Object** field in parent_obj->obejct_fields()) {
      Object* obj = *field;
      if (obj != nullptr && _from_space->contains(obj)) {
        Object* new_obj = obj->is_forwarded() ? obj->forwardee()
                                              : evacuate(obj);
        *field = new_obj;
      }
    }
    scanned += parent_obj->size();
  } while (scanned < _to_space->top());

  swap_spaces();
}

address Semispace::contains(address obj) {
  return _bottom <= obj && obj < _top;
}

address Semispace::allocate(size_t size) {
  if (_top + size <= end) {
    address obj = _top;
    _top += size;
  } else {
    return nullptr;
  }
}
