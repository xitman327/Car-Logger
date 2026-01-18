#include "user.h"
#define ENABLE_USER_CONFIG
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#define ENABLE_FS
// #include "FirebaseFS.h"
#include <FirebaseClient.h>
#include <ExampleFunctions.h>

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS, 300 /* expired in 300 sec */);

FirebaseApp app;

SSL_CLIENT ssl_client;

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

void db_init(){
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    set_ssl_client_insecure_and_buffer(ssl_client);

    Serial.println("Initializing app...");
    initializeApp(aClient, app, getAuth(user_auth), auth_debug_print, "🔐 authTask");

    // Or intialize the app and wait.
    // initializeApp(aClient, app, getAuth(user_auth), 120 * 1000, auth_debug_print);

    // Disable auto authentication
    // From UserAuth class constructor defined above, the expire period was 300 seconds (5 min).
    // Then the library will not re-authenticate after 300 seconds or auth token was expired (60 minutes).
    // The auth token will be expired in 60 minutes.
    app.autoAuthenticate(false);
}

void db_loop(){
    app.loop();
}