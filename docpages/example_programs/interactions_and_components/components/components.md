\page components Using Button Components

Discord's newest features support sending buttons alongside messages, which when clicked by the user trigger an interaction which is routed by D++ as an `on_button_click` event. To make use of this, use this code as in this example.

\include{cpp} components.cpp

When the feature is functioning, the code below will produce buttons on the reply message like in the image below:

\image html button.png
