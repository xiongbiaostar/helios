#pragma once

#include <IBinaryTreeNode.h>

#include <deque>
#include <iterator>



using std::deque;

/**
 * @author Alberto M. Esmoris Pena
 * @version 1.0
 *
 * @brief Fast depth first iterator for binary tree like nodes
 *
 *
 * Depth first node visiting order is illustrated below:
 * <table>
 * <tr> <td colspan=20 align=center>0</td> </tr>
 * <tr>
 *  <td colspan=5></td>
 *  <td align=center>1</td>
 *  <td colspan=10></td>
 *  <td align=center>4</td>
 *  <td colspan=3></td>
 * </tr>
 * <tr>
 *  <td colspan=3></td>
 *  <td>2</td>
 *  <td colspan=3></td>
 *  <td>3</td>
 *  <td colspan=6></td>
 *  <td>5</td>
 *  <td colspan=3></td>
 *  <td>6</td>
 *  <td colspan=3></td>
 * </tr>
 * </table>
 *
 * @tparam NodeType Type of binary tree node
 */
template <typename NodeType>
class BinaryTreeFastDepthIterator{
private:
    // ***  SERIALIZATION  *** //
    // *********************** //
    friend class boost::serialization::access;
    /**
     * @brief Serialize a BinaryTreeFastDepthIterator to a stream of bytes
     * @tparam Archive Type of rendering
     * @param ar Specific rendering for the stream of bytes
     * @param version Version number for the BinaryTreeFastDepthIterator
     */
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar &pendingNodes;
    }

protected:
    // ***  ATTRIBUTES  *** //
    // ******************** //
    /**
     * @brief Double ended queue used as a stack to handle nodes visiting
     */
    deque<IBinaryTreeNode<NodeType>*> pendingNodes;

public:
    // ***  CONSTRUCTION / DESTRUCTION  *** //
    // ************************************ //
    /**
     * @brief Default constructor for binary tree fast depth iterator
     */
    BinaryTreeFastDepthIterator() = default;
    /**
     * @brief Construct a binary tree fast depth iterator calling the start
     *  method with given node
     * @param node Node to start depth iterating from
     * @see BinaryTreeFastDepthIterator::start
     */
    BinaryTreeFastDepthIterator(NodeType *node) : BinaryTreeFastDepthIterator()
    {start(node);}
    virtual ~BinaryTreeFastDepthIterator() {}

    // ***  GETTERs and SETTERs  *** //
    // ***************************** //
    /**
     * @brief Start the iterator so the first visited node will be given one.
     * It is, when calling next, given node will be returned
     * @param node Node to start depth iterating from
     */
    inline void start(NodeType *node){
        pendingNodes.clear();
        pendingNodes.push_back(node);
    }
    /**
     * @brief Check if the iterator has more nodes to visit (true) or not
     *  (false)
     * @return True if there are nodes left to be visited, false otherwise
     */
    inline bool hasNext() const {return !pendingNodes.empty();}
    /**
     * @brief Obtain the next node according to depth iteration criterion
     * @return Next node according to depth iteration criterion
     */
    inline NodeType *next(){
        IBinaryTreeNode<NodeType> * node = pendingNodes.back();
        pendingNodes.pop_back();
        if(node->getRightChild() != nullptr)
            pendingNodes.push_back(node->getRightChild());
        if(node->getLeftChild() != nullptr)
            pendingNodes.push_back(node->getLeftChild());
        return node;
    }
};
