"""
Module to build a hierarchical categories of collections

TreeCollection is a hierarchical structure, where a collection is recursively
broken into sub-collections, based on arbitrary classifiers.
The sub-collections are stored as children of the source collection
and will only contain a subset of object in the source collection.

The TreeCollection starts with a collection at the root of the hierarchy.
A list of classifiers is used to recursively build children nodes, by categorizing
collection associated with parent nodes at each level of the hierarchy.

CompositeTreeCollection works similar to TreeCollection, but stores multiple tagged collections
at nodes of the hierarchy.

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from collections import OrderedDict

class TreeCollection(object):
  """
  A datastructure to build hierarchical classification of collections

  The tree is constructed with complete collection at the root of the hierarchy.
  Each level is build with children, which hold a subset of the collection at the parent node.
  """

  class Node(object):
    """
    Node of a tree collection

    Each node is associated with a collection and a list of child nodes

    """

    def __init__(self, collection, ancestry=None):
      self.collection = collection
      self.children = OrderedDict()
      self.ancestry = ancestry if ancestry else []

    def addChild(self, name, subCollection):
      """
      Adds a new Node with subcollection as a child to this node

      :param name: Name of the child node
      :param subCollection: subcollection for the child node

      """
      ancestry = list(self.ancestry)
      ancestry.append(name)
      child = TreeCollection.Node(subCollection, ancestry)
      self.children.update({name : child})
      return child

    def __repr__(self, level=0):
      """Returns string representation of a Tree Collection Node"""
      indent = '\t' * level if level else ''
      nodeStr = 'Node (item count - {} | children - {})\n'.format(len(self.collection), len(self.children))
      i = 1
      for name, child in self.children.items():
        nodeStr = nodeStr + indent + '{}. child[{}] -> '.format(i, name) + child.__repr__(level + 1) + '\n'
        i = i + 1
      return nodeStr

  def __init__(self, name, root):
    self.name = name
    self.root = root

  def getNode(self, path=None):
    """
    Returns node at a given path

    :param path: Path to lookup (Default value = None)

    """
    node = self.root
    path = path if path else []
    for name in path:
      if name in node.children:
        node = node.children[name]
      else:
        return None
    return node

  def getCollection(self, path=None):
    """
    Performs a lookup and returns collection, at the given path

    :param path: path is a list of strings, that will be followed in hierarchieal order to
                 locate the collection (Default value = None)

    """
    path = path if path else []
    node = self.getNode(path)
    return node.collection if node else None

  def getChildren(self, path=None):
    """
    Returns child node at a given path

    :param path: Path to lookup (Default value = None)


    """
    path = path if path else []
    node = self.getNode(path)
    return node.children if node else None

  def __repr__(self):
    """Returns string representation of a Tree Collection"""
    return 'Tree {}\nRoot {}'.format(self.name, str(self.root))

class CompositeTreeCollection(object):
  """
  A datastructure to build hierarchical classification of plurality of collections

  The tree is constructed with plurality of complete collections at the root of the hierarchy.
  Each level is build with children, which hold a subcollection for each of the collection at parent node.

  """

  class CompositeNode(object):
    """
    Node in a composite tree collection

    Each Node is associated with a dictionary of collections and a list of children

    """

    def __init__(self, ancestry=None):
      self.collectionMap = OrderedDict()
      self.children = OrderedDict()
      self.ancestry = ancestry if ancestry else []

    def addNode(self, treeName, node):
      """
      Adds a Tree collection Node and it's children to current node, using treeName as collection's name
            This would recursively add children of tree node to children of self

      :param treeName: Name of the tree
      :param node: Tree collection to be added

      """
      if treeName in self.collectionMap:
        raise Exception('collection {} already exists in Composite Node {}'.format(treeName, self))

      self.collectionMap.update({treeName : node.collection})

      for childName, child in node.children.items():
        if childName in self.children:
          compositeChild = self.children[childName]
        else:
          ancestry = list(self.ancestry)
          ancestry.append(childName)
          compositeChild = CompositeTreeCollection.CompositeNode(ancestry)
          self.children.update({childName : compositeChild})
        compositeChild.addNode(treeName, child)

    def __repr__(self, level=0):
      """Returns string representation of a Composite Tree Collection Node"""
      indent = '\t' * level if level else ''
      itemCountMap = {name : len(collection) for name, collection in self.collectionMap.items()}
      nodeStr = 'CompositeNode (item counts - {} | children - {})\n'.format(itemCountMap, len(self.children))
      i = 1
      for name, child in self.children.items():
        nodeStr = nodeStr + indent + '{}. child[{}] -> '.format(i, name) + child.__repr__(level + 1) + '\n'
        i = i + 1
      return nodeStr

  def __init__(self):
    self.treeNames = set()
    self.root = CompositeTreeCollection.CompositeNode()

  def addTree(self, tree):
    """
    Adds a Tree collection to self

    :param tree: Tree collection to be added

    """
    if tree.name in self.treeNames:
      raise Exception('Tree {} already exists in Composite Tree {}'.format(tree, self))

    self.treeNames.add(tree.name)
    self.root.addNode(tree.name, tree.root)

  def getCollectionMap(self, path):
    """
    Performs a lookup and returns collection map (composite collection), following the path

    :param path: path is a list of strings, that will be followed in hierarchieal order to
                 locate the collection

    """
    node = self.root
    for name in path:
      if name in node.children:
        node = node.children[name]
      else:
        return None
    return node.collectionMap

  def __repr__(self):
    """Returns string representation of a Composite Tree Collection"""
    return 'Composite Tree [{}]\nRoot {}'.format(self.treeNames, str(self.root))

class TreeCollectionFactory(object):
  """A factory class to build Tree and Composite Tree collections"""

  @staticmethod
  def buildTreeCollection(name, collection, classifiers):
    """
    Builds a tree collection from base colleciton using a list of classifiers

    :param name: name of the new Tree Collection
    :param collection: The base collection to be classified to build the hierarchical tree collection
    :param classifiers: The list of classifiers that define the hierarchy

    """
    root = TreeCollection.Node(collection)
    nodes = [root]
    for classifier in classifiers:
      nodes = TreeCollectionFactory.makeChildNodes(nodes, classifier)
    return TreeCollection(name, root)

  @staticmethod
  def makeChildNodes(nodes, classifier):
    """
    A utility method to build child nodes with given classifier

    :param nodes: List of nodes to be classified
    :param classifier: callable used or classification

    """
    childNodes = []
    for node in nodes:
      subCollectionMap = classifier(node.collection, node.ancestry)
      for name, subCollection in subCollectionMap.items():
        child = node.addChild(name, subCollection)
        childNodes.append(child)
    return childNodes

  @staticmethod
  def buildCompositeTreeCollection(collectionsMap, treeClassifiers):
    """
    Builds a composite tree collection by applying classifiers on the given collectionsMap

    :param collectionsMap: A map of collections to be classified
    :param treeClassifiers: callable used or classification

    """
    compositeTree = CompositeTreeCollection()
    for name, collection in collectionsMap.items():
      tree = TreeCollectionFactory.buildTreeCollection(name, collection, treeClassifiers)
      compositeTree.addTree(tree)
    return compositeTree
