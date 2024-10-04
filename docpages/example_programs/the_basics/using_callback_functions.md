\page callback-functions Using Callback Functions

When you create or get an object from Discord, you send the request to its API and in return you get either an error or the object you requested/created. You can pass a function to API calls as the callback function. This means that when the request completes, and you get a response from the API, your callback function executes. You must be careful with lambda captures! Good practice would be not capturing variables by reference unless you have to, since when the request completes and the function executes, the variables can already be destructed. Advanced reference can be found [here](https://dpp.dev/lambdas-and-locals.html). Now, let's see callback functions in action:

\include{cpp} callbacks.cpp

This is the result:

\image html callback_functions.png