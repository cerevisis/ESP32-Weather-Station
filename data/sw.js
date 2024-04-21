var cacheName = 'weatherstationcache';
var filesToCache = [
	"",
	"index.html",
	"setup.html",
	"update.js",
	"chart.js",
	"sw.js",
	"192x192-splash.png"
];


self.addEventListener("install", e => {
	console.log("Installed");
	e.waitUntil(
		caches.open("static").then(cache => {
			console.log("Files cached");
			return cache.addAll(["style.css", "192x192-splash.png", "index.html"]);

		})
	);
});

self.addEventListener("fetch", e => {
	console.log("Intercepting fetch request for: ${e.request.url}");
});

