/*
Direct Method implementation for Azure Sphere


    // The dmInitFunction if defined will be called at powerup from the dmInit() routine
    typedef sig_atomic_t (*dmInitFunction)(void*);

    // The dmHandler takes the payload to process and returns a an HTTP result and a pointer to a response message on the heap
    typedef int (*dmHandler)(JSON_Object *JsonPayloadObj, size_t payloadSize, char* responsePayload);

    // The dmCleanup handler is called at system exit to cleanup/release any system resources
    typedef void (*dmCleanup)(void);


*/

#include "direct_methods.h"
#include "exit_codes.h"
#include <stdlib.h>

// Define each device twin key that we plan to catch, process, and send reported property for.
// .name - The direct method name
// .DirectMethodHandler - The handler that will be called for this direct method.  The function must have the same signaure 
// void <yourFunctionName>(void* thisTwinPtr, JSON_Object *desiredProperties);
direct_method_t dmArray[] = {
	{.dmName = "test",.dmInit=(dmTestInitFunction),.dmHandler=(dmTestHandlerFunction),.dmCleanup=(dmTestCleanupFunction)}
//	{.name = "setSensorPollTime",.DirectMethodHandler = (setSensorPollTimeFunction)},
};

// Calculate how many twin_t items are in the array.  We use this to iterate through the structure.
int dmArraySize = sizeof(dmArray)/sizeof(direct_method_t);

/// <summary>
///     InitDirectMethods(void)
///     Traverse the direct method table and call the init routine if defined
/// </summary>
sig_atomic_t InitDirectMethods(void){

ExitCode result = ExitCode_Success;

    // Traverse the DM table, call the init routine if defined
    for (int i = 0; i < dmArraySize; i++)
    {
        // If this entry has an init routine call it
        if (dmArray[i].dmInit != NULL) {
            result = dmArray[i].dmInit(&dmArray[i]);
            if(result != ExitCode_Success){
                return result;
            }
        }
    }

    return result;
}

void CleanupDirectMethods(void){

    // Traverse the DM table, call the cleanup routine if defined
    for (int i = 0; i < dmArraySize; i++)
    {
        // If this entry has an init routine call it
        if (dmArray[i].dmInit != NULL) {
            dmArray[i].dmCleanup();
        }
    }
}

/// <summary>
///     Direct Method callback function, called when a Direct Method call is received from the Azure
///     IoT Hub.
/// </summary>
/// <param name="methodName">The name of the method being called.</param>
/// <param name="payload">The payload of the method.</param>
/// <param name="responsePayload">The response payload content. This must be a heap-allocated
/// string, 'free' will be called on this buffer by the Azure IoT Hub SDK.</param>
/// <param name="responsePayloadSize">The size of the response payload content.</param>
/// <returns>200 HTTP status code if the method name is reconginized and the payload is correctly parsed;
/// 400 HTTP status code if the payload is invalid;</returns>
/// 404 HTTP status code if the method name is unknown.</returns>
int DeviceMethodCallback(const char *methodName, const unsigned char *payload, size_t payloadSize, 
                         unsigned char **responsePayload, size_t *responsePayloadSize, void *userContextCallback)
{

    size_t mallocSize = 0;
    static const char errorResponse[] = "\"Method %s not found or invalid payload\"";
    static const char successResponse[] = "\"Success\"";

    // HTTP status code method name is unknown
    int result = 404;
    
    // Pointer to a response message.  Each direct method implementation will 
    // create its specific response on the heap and return the pointer.  This
    // routine will pass that pointer back to the system where the message will
    // be sent to the IoTHub and then the memory will be freed
    char* responseMsg = NULL;

    // Pointer to a copy of the passed in payload.  We'll malloc memory for this
    // data and null terminate it so that the parson library can help us process
    // the JSON message.
    char* directMethodPayload;

    // Pointer to the parsable JSON payload
    JSON_Value* payloadJson = NULL;
    JSON_Object* payloadJsonObj = NULL;

    Log_Debug("Received Device Method callback: Method name %s.\n", methodName);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Step1: Prepare the JSON payload for processing
    //
    /////////////////////////////////////////////////////////////////////////////

    // Copy the payload on to the heap then null terminate it
    // The maximum size direct method payload is 128K
    directMethodPayload = malloc(payloadSize+1);
	memcpy(directMethodPayload, payload, payloadSize);
	directMethodPayload[payloadSize] = '\0';
	
    // Note this call allocates memory and must be freed by calling json_value_free(payloadJson)
    payloadJson = json_parse_string(directMethodPayload);
	
    // Verify we have a valid JSON string from the payload
	if (payloadJson == NULL) {
        result = 400;
        goto payloadError;
	}

	// Verify that the payloadJson contains a valid JSON object
	payloadJsonObj = json_value_get_object(payloadJson);
	if (payloadJsonObj == NULL) {
		goto payloadError;
	}

    /////////////////////////////////////////////////////////////////////////////
    //
    // Step2: Find the direct method in the dm array and call the handler
    //
    /////////////////////////////////////////////////////////////////////////////


    // Traverse the DM array looking for a matching entry for the method called.  We assume that there is
    // only one matching entry in the table for this method and exit the loop after finding/calling the methods
    // handler function.
    for (int i = 0; i < dmArraySize; i++)
    {
        if (strcmp(methodName, dmArray[i].dmName) == 0) {
            
            // Call the handler for this entry
            result = dmArray[i].dmHandler(payloadJsonObj, payloadSize, responseMsg);
            break;
        }
    }
    
    /////////////////////////////////////////////////////////////////////////////
    //
    // Step3: Make sure there is a response to return to the Azure IoT library
    //
    /////////////////////////////////////////////////////////////////////////////

    // Look at the result code to determine how to proceed
    switch(result){

    // 200 HTTP status code if the method name is reconginized and the payload is correctly parsed;
    case 200:
        // Check to see if the direct method handler created a response message on the heap, if so 
        // then update the response payload variables to return to the Azure IoT library
        if(responseMsg != NULL){

            // Update the "pass value by reference" variables
            *responsePayloadSize = sizeof(responseMsg);
            *responsePayload = responseMsg;
            goto cleanup;
        }
        // The direct method did not create a response message, but it returned 200, so create a canned
        // success message to return to the Azure IoT library
        else{

		// Construct the response message.  This response will be displayed in the cloud when calling the direct method
        // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
		mallocSize = strlen(successResponse) + strlen(methodName);
        *responsePayload = malloc(mallocSize);
        if (*responsePayload == NULL) {

			Log_Debug("ERROR: SetupHeapMessage error.\n");
		    exitCode = ExitCode_NoMethodFound_Malloc_failed;
            abort();
		}
        
        // Construct the response message
        *responsePayloadSize = (size_t)snprintf(*responsePayload, mallocSize, successResponse);
        goto cleanup;

        }
    
    // If the direct method handler returned one of the error cases, then generate a canned error
    // response.
    case 400: // 400 HTTP status code if the payload is invalid
    case 404: // 404 HTTP status code if the method name is unknown
    default:
        goto payloadError;
        break;
    }
    
payloadError:
	// Construct the response message.  This response will be displayed in the cloud when calling the direct method
    // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
	mallocSize = strlen(errorResponse) + strlen(methodName);
    *responsePayload = malloc(mallocSize);
    if (*responsePayload == NULL) {

		Log_Debug("ERROR: SetupHeapMessage error.\n");
		exitCode = ExitCode_NoMethodFound_Malloc_failed;
        abort();
    }
        
    // Construct the response message
    *responsePayloadSize = (size_t)snprintf(*responsePayload, mallocSize, errorResponse, methodName);

cleanup:    
    // Release the memory allocated by this routine
    free (directMethodPayload);
    json_value_free(payloadJson);
    
    return result;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//  Functions for example directMethod
//
//////////////////////////////////////////////////////////////////////////////////////


// The dmInitFunction if defined will be called at powerup from the dmInit() routine
sig_atomic_t dmTestInitFunction(void* thisDmEntry){

    direct_method_t* thisDmArrayEntry = (direct_method_t*)thisDmEntry;
    Log_Debug("%s DirectMethod initFunction Called\n", thisDmArrayEntry->dmName);
    return ExitCode_Success;
}

// The dmHandler takes the payload to process and returns a pointer to a response message on the heap
int dmTestHandlerFunction(JSON_Object *JsonPayloadObj, size_t payloadSize, char* responsePayload){

    // Assume that the routine will return success
    int result;

	// Verify that the payloadJson contains a valid JSON object
	if (JsonPayloadObj != NULL) {

        // Pull the Key: value pair from the JSON object, we're looking for {"returnVal": <integer>}
	    // Verify that the new timer is < 0
    	result  = (int)json_object_get_number(JsonPayloadObj, "returnVal");
        if (result < 1) {
		
            // There was no payload, return succes for this test routine
            result = 200;
	    }

	}
    else{
        // There was not a valid payload, just return success for this test method
        result = 200;
    }

    return result;
    
}

// The dmCleanup handler is called at system exit to cleanup/release any system resources
void dmTestCleanupFunction(void){

    Log_Debug("DirectMethod cleanup called\n");

}

