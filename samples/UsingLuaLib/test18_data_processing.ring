/*
	LuaLib Demo 18 - Data processing
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 18: Data processing
# -------------------------------------------------------
? "Demo 18: Data processing..."

oLua = new LuaState

# -------------------------------------------------------
# CSV-like data processing in Lua
# -------------------------------------------------------
? "--- CSV Data Processing ---"

oLua.setString("csv_data",
	"Name,Age,Score" + nl +
	"User 1,25,92" + nl +
	"User 2,30,85" + nl +
	"User 3,22,97" + nl +
	"User 4,28,88" + nl +
	"User 5,35,91"
)

oLua.doString('
	-- Parse CSV data
	records = {}
	local lines = {}
	for line in csv_data:gmatch("[^\n]+") do
		lines[#lines + 1] = line
	end

	-- Extract headers
	headers = {}
	for header in lines[1]:gmatch("[^,]+") do
		headers[#headers + 1] = header
	end

	-- Parse records
	for i = 2, #lines do
		local record = {}
		local col = 1
		for value in lines[i]:gmatch("[^,]+") do
			record[headers[col]] = value
			col = col + 1
		end
		records[#records + 1] = record
	end

	-- Analysis
	total_score = 0
	max_score = 0
	max_name = ""
	min_score = 999
	min_name = ""
	count = #records

	for _, r in ipairs(records) do
		local score = tonumber(r.Score)
		total_score = total_score + score
		if score > max_score then
			max_score = score
			max_name = r.Name
		end
		if score < min_score then
			min_score = score
			min_name = r.Name
		end
	end

	avg_score = total_score / count

	-- Sort by score (descending)
	table.sort(records, function(a, b)
		return tonumber(a.Score) > tonumber(b.Score)
	end)

	-- Build ranking
	ranking = {}
	for i, r in ipairs(records) do
		ranking[i] = i .. ". " .. r.Name .. " (Score: " .. r.Score .. ")"
	end
')

? "Total records: " + oLua.getNumber("count")
? "Average score: " + oLua.getNumber("avg_score")
? "Highest score: " + oLua.getString("max_name") + " (" + oLua.getNumber("max_score") + ")"
? "Lowest score:  " + oLua.getString("min_name") + " (" + oLua.getNumber("min_score") + ")"
? ""

? "Rankings:"
aRanking = oLua.getTable("ranking")
nCount = len(aRanking)
for nItem = 1 to nCount
	? "  " + aRanking[nItem][2]
next
? ""

# -------------------------------------------------------
# Building a report object in Lua
# -------------------------------------------------------
? "--- Report Building ---"

oLua.doString('
	function buildReport(title, items)
		local report = {
			title = title,
			count = #items,
			summary = ""
		}
		local total = 0
		for _, item in ipairs(items) do
			total = total + item
		end
		report.summary = "Total: " .. total .. ", Average: " ..
			string.format("%.2f", total / #items)
		return report
	end
')

aResult = oLua.callFunction("buildReport", ["Monthly Sales", [150, 230, 180, 290, 310, 270]])
aReport = aResult[1]
? "Report from Lua:"
nCount = len(aReport)
for nItem = 1 to nCount
	? "  " + aReport[nItem][1] + ": " + aReport[nItem][2]
next
? ""

# -------------------------------------------------------
# Pattern matching (Lua string library)
# -------------------------------------------------------
? "--- Pattern Matching ---"

oLua.setString("text", "Contact us at user1@example.com or user2@example.org for help. Visit https://example.com/help")

oLua.doString('
	emails = {}
	for email in text:gmatch("[%w%.%-]+@[%w%.%-]+%.[%w]+") do
		emails[#emails + 1] = email
	end

	urls = {}
	for url in text:gmatch("https?://[%w%.%-/]+") do
		urls[#urls + 1] = url
	end
')

? "Emails found:"
aEmails = oLua.getTable("emails")
nCount = len(aEmails)
for nItem = 1 to nCount
	? "  " + aEmails[nItem][2]
next

? "URLs found:"
aURLs = oLua.getTable("urls")
nCount = len(aURLs)
for nItem = 1 to nCount
	? "  " + aURLs[nItem][2]
next

oLua.close()
? ""
