\page modal-dialog-interactions Modals

Modal dialog interactions are a new Discord API feature that allow you to have pop-up windows which prompt the user to input information. Once the user has filled in this information, your program will receive an `on_form_submit` event which will contain the data which was input. You must use a slash command interaction response to submit your modal form data to Discord, via the `on_slashcommand` event. From here calling the `dialog` method of the `interaction_create_t` event object will trigger the dialog to appear.

Each dialog box may have up to five rows of input fields. The example below demonstrates a simple setup with just one text input:

\include{cpp} modal_dialog_interactions.cpp

If you compile and run this program and wait for the global command to register, typing `/dialog` will present you with a dialog box like the one below:

\image html modal_dialog.png

