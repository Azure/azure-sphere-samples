/*

MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Brian Willess

This implementation was inspired by GitHub project: https://gist.github.com/mycodeschool/7429492

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/


/* Doubly Linked List implementation */

#include "linkedList.h"

#if defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    

// Initialize the list
void InitLinkedList(void){
	head = NULL; // empty list. set head as NULL. 
}

//Creates a new Node and returns pointer to it. 
telemetryNode_t* GetNewNode(const char* telemetryJson, size_t stringLen) {

	int nodeSize = sizeof(telemetryNode_t) + stringLen + 1;

	telemetryNode_t* newNode = (telemetryNode_t*)malloc(nodeSize);

	// Verify we were able to allocate memory for the new node, if not
	// then set exitCode to reflect the erro.  The main loop will see this
	// change and exit.
	if(newNode == NULL){
		exitCode = ExitCode_AddTelemetry_Malloc_Failed;
	}

	newNode->prev = NULL;
	newNode->next = NULL;
	strncpy (newNode->telemetryJson, telemetryJson, stringLen);
	newNode->telemetryJson[stringLen] = '\0';
	return newNode;
}

//Inserts a Node at head of doubly linked list
telemetryNode_t* InsertAtHead(char* x, int stringLen) {
	telemetryNode_t* newNode = GetNewNode(x, stringLen);
	if(head == NULL) {
		head = newNode;
		return head;
	}
	else{
		head->prev = newNode;
		newNode->next = head; 
		head = newNode;
	}
	return head;
}

//Inserts a Node at tail of Doubly linked list
telemetryNode_t* InsertAtTail(char* x, int stringLen) {
	telemetryNode_t* temp = head;
	telemetryNode_t* newNode = GetNewNode(x, stringLen);
	if(head == NULL) {
		head = newNode;
		return head;
	}
	while(temp->next != NULL) temp = temp->next; // Go To last Node
	temp->next = newNode;
	newNode->prev = temp;
	return newNode;
}

// Remove a Node from the list
bool DeleteNode(telemetryNode_t* nodeToRemove){

    // Traverse the list to find the node to remove
	telemetryNode_t* temp = head;
	while(temp != NULL) {
        if(temp == nodeToRemove){
            
            // Handle case where we remove the head Node
            if(temp == head){
                head = temp->next;
                
				// Handle the case where we remove the only node in the list
				if(head != NULL){
					temp->next->prev = NULL;
				}
            }

            // Handle case where we remove the tail Node
            else if(temp->next == NULL){
                temp->prev->next = NULL;
            }

            // Remove a Node in the middle of the list somewhere
            else {
             
                temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
            }

            // We just found the Node to remove and reset pointers in the 
            // rest of the list to orphan this node, zap it and return true.
			Log_Debug("Removing node at address %x\n", temp);
			free(temp);
            return true;

        }
		// Look at the next node in the list
		temp = temp->next;
	}
    return false;
}

//Prints all the elements in linked list in forward traversal order
void Print() {
	telemetryNode_t* temp = head;
	Log_Debug("Forward: ");
	while(temp != NULL) {
		Log_Debug("%s ",temp->telemetryJson);
		temp = temp->next;
	}
	Log_Debug("\n");
}

//Prints all elements in linked list in reverse traversal order. 
void ReversePrint() {
	telemetryNode_t* temp = head;
	
	if(temp == NULL){ // empty list, exit
		return;
	}

	// Going to last Node
	while(temp->next != NULL) {
		temp = temp->next;
	}
	// Traversing backward using prev pointer
	Log_Debug("Reverse: ");
	while(temp != NULL) {
		Log_Debug("%s ",temp->telemetryJson);
		temp = temp->prev;
	}
	Log_Debug("\n");
}

// Remove all the nodes in the list
void DeleteEntireList(void){

	telemetryNode_t* temp = head;
	telemetryNode_t* deleteMe = NULL;
	if(temp == NULL){
		return; // empty list, exit
	}		
	
	// Traverse the list deleteing Nodes from the start to the end
	do{
		deleteMe = temp;
		temp = temp->next;
		DeleteNode(deleteMe);
	} while (temp != NULL);

}

#endif // defined(IOT_HUB_APPLICATION) && defined(ENABLE_TELEMETRY_RESEND_LOGIC)    

#ifdef REMOVE
int main() {

    InitLinkedList();

    telemetryNode_t* node1 = NULL;
    telemetryNode_t* node2 = NULL;
    telemetryNode_t* node3 = NULL;
    telemetryNode_t* node4 = NULL;
    telemetryNode_t* node5 = NULL;


    node5 = InsertAtTail("testing\0", sizeof("testing\0")); Print();
    DeleteNode(node5); Print();

	node1 = InsertAtTail("{testing\": 1}", sizeof("{testing\": 1}")); Print();
	node2 = InsertAtTail("{testingaaa\": 2}", sizeof("{testingaaa\": 2}")); Print();
	node3 = InsertAtTail("{testingbbbbbbb\": 3}", sizeof("{testingbbbbbbb\": 3}")); Print();
	node4 = InsertAtTail("{testingcccccccccc\": 4}", sizeof("{testingcccccccccc\": 4}")); Print();
    node5 = InsertAtTail("{testingdddddddddddddd\": 5}", sizeof("{testingdddddddddddddd\": 5}")); Print();

    DeleteNode(node1); Print();
    DeleteNode(node2); Print();
    DeleteNode(node3); Print();
    DeleteNode(node4); Print();
    DeleteNode(node5); Print();

    DeleteEntireList(); Print();

	
}
#endif 