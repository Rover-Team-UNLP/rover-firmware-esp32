#include "http_portal.h"

static esp_err_t get_handler(httpd_req_t *);
static esp_err_t post_handler(httpd_req_t *);

static const char *HTML_FORM =
    "<!DOCTYPE html>"
    "<html lang=\"es\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">"
    "<title>Configuración WiFi | Rover Autónomo</title>"
    "<style>"
    ":root {"
    "--eng-blue: #007cc2;"
    "--eng-dark: #005a8d;"
    "--bg-color: #f0f2f5;"
    "--card-bg: #ffffff;"
    "--text-main: #333333;"
    "--text-secondary: #666666;"
    "--input-border: #d1d5db;"
    "--shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);"
    "}"
    "* { box-sizing: border-box; margin: 0; padding: 0; }"
    "body {"
    "font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, Helvetica, Arial, sans-serif;"
    "background-color: var(--bg-color);"
    "color: var(--text-main);"
    "display: flex;"
    "justify-content: center;"
    "align-items: center;"
    "min-height: 100vh;"
    "padding: 20px;"
    "}"
    ".container {"
    "background-color: var(--card-bg);"
    "width: 100%;"
    "max-width: 380px;"
    "border-radius: 16px;"
    "box-shadow: var(--shadow);"
    "overflow: hidden;"
    "display: flex;"
    "flex-direction: column;"
    "border: 1px solid rgba(255,255,255,0.5);"
    "}"
    ".header {"
    "background: linear-gradient(135deg, var(--eng-blue) 0%, var(--eng-dark) 100%);"
    "padding: 40px 20px;"
    "text-align: center;"
    "color: white;"
    "position: relative;"
    "}"
    ".header h1 { font-size: 24px; font-weight: 700; margin-bottom: 6px; letter-spacing: -0.5px; }"
    ".header p { font-size: 14px; opacity: 0.95; font-weight: 400; letter-spacing: 0.5px; text-transform: uppercase; }"
    ".form-content { padding: 30px; }"
    ".input-group { margin-bottom: 20px; }"
    "label {"
    "display: block;"
    "font-size: 12px;"
    "color: var(--text-secondary);"
    "margin-bottom: 8px;"
    "font-weight: 700;"
    "text-transform: uppercase;"
    "letter-spacing: 0.8px;"
    "}"
    "input[type=\"text\"], input[type=\"password\"] {"
    "width: 100%;"
    "padding: 14px;"
    "font-size: 16px;"
    "border: 2px solid var(--bg-color);"
    "background-color: #f8fafc;"
    "border-radius: 8px;"
    "transition: all 0.2s ease;"
    "outline: none;"
    "color: var(--text-main);"
    "}"
    "input:focus {"
    "border-color: var(--eng-blue);"
    "background-color: #fff;"
    "box-shadow: 0 0 0 4px rgba(0, 124, 194, 0.1);"
    "}"
    "button {"
    "width: 100%;"
    "padding: 16px;"
    "background-color: var(--eng-blue);"
    "color: white;"
    "border: none;"
    "border-radius: 8px;"
    "font-size: 16px;"
    "font-weight: 600;"
    "cursor: pointer;"
    "transition: transform 0.1s, background-color 0.2s;"
    "margin-top: 15px;"
    "box-shadow: 0 4px 12px rgba(0, 124, 194, 0.3);"
    "}"
    "button:hover { background-color: var(--eng-dark); }"
    "button:active { transform: scale(0.98); }"
    ".footer {"
    "text-align: center;"
    "margin-top: 30px;"
    "font-size: 11px;"
    "color: #bdc3c7;"
    "border-top: 1px solid #f1f1f1;"
    "padding-top: 20px;"
    "}"
    ".footer strong { color: var(--eng-blue); font-weight: 600; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<div class=\"header\">"
    "<h1>Conexión WiFi</h1>"
    "<p>Rover autónomo</p>"
    "</div>"
    "<div class=\"form-content\">"
    "<form action=\"/save\" method=\"post\">"
    "<div class=\"input-group\">"
    "<label for=\"ssid\">Red WiFi (SSID)</label>"
    "<input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"Ej: Barcala\" required autocomplete=\"off\">"
    "</div>"
    "<div class=\"input-group\">"
    "<label for=\"password\">Contraseña</label>"
    "<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"••••••••\" required>"
    "</div>"
    "<button type=\"submit\">Guardar y Conectar</button>"
    "</form>"
    "<div class=\"footer\">"
    "Facultad de Ingeniería<br>"
    "<strong>UNLP 2026</strong>"
    "</div>"
    "</div>"
    "</div>"
    "</body>"
    "</html>";
static httpd_handle_t server_handler;
static const httpd_uri_t get_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL,
};
static const httpd_uri_t post_uri = {
    .uri = "/save",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL,
};

server_start_return_t http_portal_start()
{
    httpd_config_t default_conf = HTTPD_DEFAULT_CONFIG();
    esp_err_t err = httpd_start(&server_handler, &default_conf);
    if (err == ESP_OK)
    {
        err = httpd_register_uri_handler(server_handler, &get_uri);
        if (err == ESP_OK)
        {
            err = httpd_register_uri_handler(server_handler, &post_uri);
        }
    }
    server_start_return_t ret_val = {
        .handler = server_handler,
        .return_err = err,
    };

    return ret_val;
}

void http_portal_stop(httpd_handle_t handler)
{
    httpd_stop(handler);
}

static esp_err_t get_handler(httpd_req_t *req)
{
    char *mime = "text/html";
    esp_err_t err = httpd_resp_set_type(req, mime);
    if (err == ESP_OK) {
        httpd_resp_send(req, HTML_FORM, HTTPD_RESP_USE_STRLEN);
    }
}